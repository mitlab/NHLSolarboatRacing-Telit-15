
/* Name: Storage task
 * Description: Manages logbook on storage (microSD card)
 * Author: Casper Kloppenburg
 */

/* Includes */

#include "StorageTask.h"

#include "fat_sd/diskio.h"
#include "fat_sd/ff.h"

/* Defines */

#define STORAGE_RING_BUFFER_SIZE		(512)

/* Structs */

typedef struct {

	char wrBuffer[STORAGE_RING_BUFFER_SIZE];
	BOOL wrBufferIsFull;
	uint32_t wrBufferHead;
	uint32_t wrBufferTail;

} STORAGE_RING_BUFFER_T;

/* Variables */

OS_MutexID _logMutexId = E_CREATE_FAIL;
OS_MutexID _canMutexId = E_CREATE_FAIL;

OS_FlagID _availableFlagId = E_CREATE_FAIL;

STORAGE_RING_BUFFER_T _logRingBuffer;
STORAGE_RING_BUFFER_T _canRingBuffer;

FATFS _fatFs;

FIL _logFile;
FIL _canFile;

/* Prototypes */

BOOL StorageTask_Init();
BOOL StorageTask_PrepareFile(char *filename, FIL *file);
void StorageTask_QueueData(STORAGE_RING_BUFFER_T *rb, uint8_t *dat, uint32_t len);
void StorageTask_FlushToDisk(STORAGE_RING_BUFFER_T *rb, FIL *file);

/* Implementation */

void StorageTask_Run(void *pdata)
{
	Debug_Send(DM_INFO, "Storage task started.");

	// Initialize administration and prepare storage
	if (!StorageTask_Init())
	{
		Debug_Send(DM_FATAL_ERROR, "Failed to initialize storage task.");
		CoExitTask();
		while (1);
	}

	for (;;)
	{
		// Wait for data in ring buffers
		CoWaitForSingleFlag(_availableFlagId, 0);

		// Flush log data from ring buffer to disk
		if (CoEnterMutexSection(_logMutexId) == E_OK)
		{
			StorageTask_FlushToDisk(&_logRingBuffer, &_logFile);
			CoLeaveMutexSection(_logMutexId);
		}

		// Flush CAN data from ring buffer to disk
		if (CoEnterMutexSection(_canMutexId) == E_OK)
		{
			StorageTask_FlushToDisk(&_canRingBuffer, &_canFile);
			CoLeaveMutexSection(_canMutexId);
		}

		f_sync(&_logFile);
	}
}

void StorageTask_WriteLogFile(char *str)
{
	if (CoEnterMutexSection(_logMutexId) == E_OK)
	{
		StorageTask_QueueData(&_logRingBuffer, (uint8_t *)str, strlen(str));
		CoLeaveMutexSection(_logMutexId);
	}
}

void StorageTask_WriteCanFile(uint8_t *dat, uint32_t len)
{
	if (CoEnterMutexSection(_canMutexId) == E_OK)
	{
		StorageTask_QueueData(&_canRingBuffer, dat, len);
		CoLeaveMutexSection(_canMutexId);
	}
}

BOOL StorageTask_Init()
{
	// Set ring buffers to default state
	_logRingBuffer.wrBufferIsFull = FALSE;
	_logRingBuffer.wrBufferHead = 0;
	_logRingBuffer.wrBufferTail = 0;
	_canRingBuffer.wrBufferIsFull = FALSE;
	_canRingBuffer.wrBufferHead = 0;
	_canRingBuffer.wrBufferTail = 0;

	// Create mutexes to protect against race-conditions
	_logMutexId = CoCreateMutex();
	if (_logMutexId == E_CREATE_FAIL)
	{
		Debug_Send(DM_FATAL_ERROR, "Log mutex creation failed.");
		return FALSE;
	}
	_canMutexId = CoCreateMutex();
	if (_canMutexId == E_CREATE_FAIL)
	{
		Debug_Send(DM_FATAL_ERROR, "CAN mutex creation failed.");
		return FALSE;
	}

	// Create availability flag (auto-reset, initial state 0)
	_availableFlagId = CoCreateFlag(1, 0);
	if (_availableFlagId == E_CREATE_FAIL)
	{
		Debug_Send(DM_FATAL_ERROR, "Storage available flag creation failed.");
		return FALSE;
	}

	// Create timer for FAT disk I/O timing
	OS_TCID timerId;
	if ((timerId = CoCreateTmr(TMR_TYPE_PERIODIC, 1, 1, disk_timerproc)) == E_CREATE_FAIL)
	{
		Debug_Send(DM_FATAL_ERROR, "Disk I/O timer creation failed.");
		return FALSE;
	}
	CoStartTmr(timerId);

	// "Mount" drive, always returns FR_OK
	f_mount(0, &_fatFs);

	Debug_Send(DM_INFO, "Preparing log file...");
	if (!StorageTask_PrepareFile(LOG_FILE_NAME, &_logFile))
		return FALSE;

	Debug_Send(DM_INFO, "Preparing CAN file...");
	if (!StorageTask_PrepareFile(CAN_FILE_NAME, &_canFile))
		return FALSE;

	return TRUE;
}

BOOL StorageTask_PrepareFile(char *filename, FIL *file)
{
	// Open file
	FRESULT res = f_open(file, filename, FA_WRITE);
	if (res == FR_NO_FILE)
	{
		// Not found, try to create the file
		Debug_Send(DM_INFO, "File not found, creating new file.");
		res = f_open(file, CAN_FILE_NAME, FA_CREATE_NEW | FA_WRITE);
		if (res != FR_OK)
		{
			// Failed to open and create file, abort
			Debug_Send(DM_FATAL_ERROR, "File creation failed.");
			return FALSE;
		}
		else
		{
			// All OK
			Debug_Send(DM_INFO, "File created successful.");
		}
	}
	else if (res != FR_OK)
	{
		// Failed to open file, abort task
		Debug_Send(DM_FATAL_ERROR, "Can not open file.");
		return FALSE;
	}

	// Seek to the end of the CAN file to append it
	if (f_lseek(file, file->fsize) != FR_OK)
	{
		Debug_Send(DM_FATAL_ERROR, "Failed to seek to the end of the file.");
		return FALSE;
	}

	Debug_Send(DM_INFO, "File prepared successfully.");

	return TRUE;
}

void StorageTask_QueueData(STORAGE_RING_BUFFER_T *rb, uint8_t *dat, uint32_t len)
{
	// Ring buffer is full (i.e. data wasn't retrieved fast enough from ring buffer)
	if (rb->wrBufferIsFull)
		return;

	while (len--)
	{
		// Put byte into ring buffer
		rb->wrBuffer[rb->wrBufferHead] = *dat++;

		// Update ring buffer administration
		rb->wrBufferHead = (rb->wrBufferHead + 1) % STORAGE_RING_BUFFER_SIZE;
		if (rb->wrBufferHead == rb->wrBufferTail)
		{
			// Ring buffer is full
			rb->wrBufferIsFull = TRUE;
			break;
		}
	}

	// Inform storage task there's data to be written
	CoSetFlag(_availableFlagId);
}

void StorageTask_FlushToDisk(STORAGE_RING_BUFFER_T *rb, FIL *file)
{
	UINT bytesWritten;

	if (rb->wrBufferTail < rb->wrBufferHead)
	{
		f_write(file, rb->wrBuffer + rb->wrBufferTail,
				rb->wrBufferHead - rb->wrBufferTail, &bytesWritten);
	}
	else if (rb->wrBufferTail > rb->wrBufferHead || (rb->wrBufferTail == rb->wrBufferHead && rb->wrBufferIsFull))
	{
		f_write(file, rb->wrBuffer + rb->wrBufferTail,
				STORAGE_RING_BUFFER_SIZE - rb->wrBufferTail, &bytesWritten);
		f_write(file, rb->wrBuffer, rb->wrBufferHead, &bytesWritten);
	}

	rb->wrBufferTail = rb->wrBufferHead;
	rb->wrBufferIsFull = FALSE;
}
