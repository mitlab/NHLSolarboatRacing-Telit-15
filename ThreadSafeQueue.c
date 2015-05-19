
/* Name: Thread-safe queue
 * Description: A thread-safe queue to be used as pipeline between threads
 * Author: Casper Kloppenburg
 */

/* Includes */

#include "ThreadSafeQueue.h"

/* Implementation */

BOOL ThreadSafeQueue_Allocate(QueueStruct_T *queue, size_t objectSize, uint32_t queueSize)
{
	// Dynamically allocate memory space
	queue->memoryBuffer = CoKmalloc(objectSize * queueSize);
	if (queue->memoryBuffer == NULL)
		return TSQ_ERROR_ALLOCATION_FAILED;

	// Create mutex for thread-safety
	queue->mutexId = CoCreateMutex();
	if (queue->mutexId == E_CREATE_FAIL)
	{
		CoKfree(queue->memoryBuffer);
		return TSQ_ERROR_ALLOCATION_FAILED;
	}

	// Assign default values
	queue->objectSize = objectSize;
	queue->queueSize = queueSize;
	queue->queueIsFull = FALSE;
	queue->queueHeadIndex = 0;
	queue->queueTailIndex = 0;

	return TSQ_OK;
}

BOOL ThreadSafeQueue_Enqueue(QueueStruct_T *queue, const void *object)
{
	CoEnterMutexSection(queue->mutexId);

	if (queue->queueIsFull == TRUE)
	{
		CoLeaveMutexSection(queue->mutexId);
		return TSQ_ERROR_QUEUE_IS_FULL;
	}

	// Copy object to queue
	memcpy(queue->memoryBuffer + queue->queueHeadIndex * queue->objectSize, object, queue->objectSize);

	// Update head index
	queue->queueHeadIndex = (queue->queueHeadIndex + 1) % queue->queueSize;
	if (queue->queueHeadIndex == queue->queueTailIndex)
		queue->queueIsFull = TRUE;

	CoLeaveMutexSection(queue->mutexId);

	return TSQ_OK;
}

BOOL ThreadSafeQueue_Dequeue(QueueStruct_T *queue, void *object)
{
	CoEnterMutexSection(queue->mutexId);

	if (queue->queueTailIndex == queue->queueHeadIndex && !queue->queueIsFull)
	{
		CoLeaveMutexSection(queue->mutexId);
		return TSQ_ERROR_QUEUE_IS_EMPTY;
	}

	// Copy object from queue
	memcpy(object, queue->memoryBuffer + queue->queueTailIndex * queue->objectSize, queue->objectSize);

	// Update tail index
	queue->queueTailIndex = (queue->queueTailIndex + 1) % queue->queueSize;

	// Queue is not completely full
	queue->queueIsFull = FALSE;

	CoLeaveMutexSection(queue->mutexId);

	return TSQ_OK;
}
