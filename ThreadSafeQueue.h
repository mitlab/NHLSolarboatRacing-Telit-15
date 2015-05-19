
/* Name: Thread-safe queue
 * Description: A thread-safe queue to be used as pipeline between threads
 * Author: Casper Kloppenburg
 */

#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

/* Includes */

#include <stdio.h>
#include <lpc_types.h>
#include <lpc17xx_clkpwr.h>
#include <lpc17xx_gpio.h>
#include <lpc17xx_libcfg_default.h>
#include <lpc17xx_uart.h>
#include <CoOs.h>

/* Enums */

typedef enum {
	TSQ_OK							= 0,
	TSQ_ERROR_ALLOCATION_FAILED		= 1,
	TSQ_ERROR_QUEUE_IS_EMPTY		= 2,
	TSQ_ERROR_QUEUE_IS_FULL			= 3,
} TSQ_RESULT;

/* Structs */

typedef struct {
	uint8_t *memoryBuffer;
	OS_MutexID mutexId;
	size_t objectSize;
	uint32_t queueSize;
	BOOL queueIsFull;
	uint32_t queueHeadIndex;
	uint32_t queueTailIndex;
} QueueStruct_T;

/* Prototypes */

TSQ_RESULT ThreadSafeQueue_Allocate(QueueStruct_T *queue, size_t objectSize, uint32_t queueSize);
TSQ_RESULT ThreadSafeQueue_Enqueue(QueueStruct_T *queue, const void *object);
TSQ_RESULT ThreadSafeQueue_Dequeue(QueueStruct_T *queue, void *object);

#endif
