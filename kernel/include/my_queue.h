#ifndef MY_QUEUE_H
#define MY_QUEUE_H

#include "my_types.h"
#include "my_list.h"


typedef struct _MY_Queue
{
	void *DataBuffer;
	myList ReaderSleepList;
	myList WriterSleepList;
	my_uint32 ReadPos;
	my_uint32 WritePos;
	my_uint32 ElementSingleSize;
	my_uint32 ElementNum;
	my_uint8 IsUsed;
}My_Queue;

typedef enum _MY_QUEUE_USED
{
	MY_QUEUE_UNUSED = 0,
	MY_QUEUE_USED
}My_Queue_Used;


void My_Queue_Init(void);
my_uint32 My_QueueCreate(my_uint32 *QueueHandle, my_uint32 ElementSingleSize, my_uint32 ElementNum);
my_uint32 My_QueueWrite(my_uint32 QueueHandle, const void *Buffer, my_uint32 Size);
my_uint32 My_QueueTryWrite(my_uint32 QueueHandle, const void *Buffer, my_uint32 Size);
my_uint32 My_QueueWriteTimeout(my_uint32 QueueHandle, const void *Buffer, my_uint32 Size, my_uint32 Timeout);

my_uint32 My_QueueRead(my_uint32 QueueHandle,  void *Buffer, my_uint32 Size);
my_uint32 My_QueueTryRead(my_uint32 QueueHandle, void *Buffer, my_uint32 Size);
my_uint32 My_QueueReadTimeout(my_uint32 QueueHandle,  void *Buffer, my_uint32 Size, my_uint32 Timeout);

my_uint32 My_QueueDestroy(my_uint32 QueueHandle);
my_uint32 My_QueueRemainingSpace(my_uint32 QueueHandle);

my_uint8 My_QueueEmpty(my_uint32 QueueHandle);
my_uint8 My_QueueFull(my_uint32 QueueHandle);

#endif
