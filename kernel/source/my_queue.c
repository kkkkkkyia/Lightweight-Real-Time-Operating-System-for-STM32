#include "my_queue.h"
#include "my_critical.h"
#include "my_config.h"
#include "my_lib.h"
#include "my_types.h"
#include "my_printk.h"
#include "my_time.h"
#include "my_error_code.h"
#include "my_scheduler.h"
#include "my_mem.h"
#include "my_arch.h"
My_Queue My_QueuePool[MY_QUEUE_MAX_NUM];
extern My_TCB * volatile CurrentTCB;

#define MY_QUEUE_HANDLE_TO_POINTER(HANDLE) &My_QueuePool[HANDLE]
#define MY_QUEUE_INDEX_TO_BUFFERADDR(QUEUE_P,INDEX) (((my_uint8*)QUEUE_P->DataBuffer)+(QUEUE_P->ElementSingleSize*INDEX))
#define My_Queue_Check_Handle_Valid(Handle)\
{\
	if (Handle > MY_QUEUE_MAX_NUM)\
		return MY_QUEUE_HANDLE_INVALID;\
}

#define My_Queue_Check_Handle_Created( Handle) \
{\
	if(My_QueuePool[Handle].IsUsed==MY_QUEUE_UNUSED)\
		return MY_QUEUE_NOT_BEEN_CREATED;\
}

static __inline void My_QueueWritePosIncrease(My_Queue* Queue)
{
	Queue->WritePos++;
	if (Queue->WritePos == Queue->ElementNum)
	{
		Queue->WritePos = 0;
	}
}
static __inline void My_QueueReadPosIncrease(My_Queue* Queue)
{
	Queue->ReadPos++;
	if (Queue->ReadPos == Queue->ElementNum)
	{
		Queue->ReadPos = 0;
	}
}
void My_Queue_Init(void)
{
	my_uint32 i;
	for (i = 0; i < MY_QUEUE_MAX_NUM; i++)
	{
		My_QueuePool[i].DataBuffer = MY_NULL;
		My_QueuePool[i].ElementNum = 0;
		My_QueuePool[i].ElementSingleSize = 0;
		My_QueuePool[i].IsUsed = MY_QUEUE_UNUSED;
		My_QueuePool[i].ReadPos = 0;
		My_QueuePool[i].WritePos = 0;
		ListInit(&My_QueuePool[i].ReaderSleepList);
		ListInit(&My_QueuePool[i].WriterSleepList);
	}
}
static my_uint32 My_GetQueueResource(my_uint32 *QueueHandle)
{
	my_uint32 i=0;
	for ( i = 0; i < MY_QUEUE_MAX_NUM; i++)
	{
		if (My_QueuePool[i].IsUsed == MY_QUEUE_UNUSED)
			break;
	}
	if (i == MY_QUEUE_MAX_NUM)
	{
		return MY_NOT_ENOUGH_MEM_FOR_QUEUE_CREATE;
	}
	else
	{
		*QueueHandle = i;
	}
	return MY_SUCCESS;
}
my_uint32 My_QueueCreate(my_uint32 *QueueHandle, my_uint32 ElementSingleSize, my_uint32 ElementNum)
{
	my_uint32 Ret = MY_SUCCESS;
	MY_CHECK_NULL_POINTER(QueueHandle);

	if (ElementSingleSize == 0 || ElementNum == 0)
	{
		return MY_QUEUE_CREATE_INVALID_PARAM;
	}

	MY_ENTER_CRITICAL();
	Ret = My_GetQueueResource(QueueHandle);
	if (Ret != MY_SUCCESS)
	{
		goto MY_QUEUE_CREATE_EXIT;
	}

	My_QueuePool[*QueueHandle].DataBuffer = My_Malloc(ElementNum*ElementSingleSize);
	if (My_QueuePool[*QueueHandle].DataBuffer == MY_NULL)
	{
		Ret = MY_NOT_ENOUGH_MEM_FOR_QUEUE_CREATE;
		goto MY_QUEUE_CREATE_EXIT;
	}
	My_QueuePool[*QueueHandle].ElementNum = ElementNum;
	My_QueuePool[*QueueHandle].ElementSingleSize = ElementSingleSize;
	My_QueuePool[*QueueHandle].ReadPos = 0;
	My_QueuePool[*QueueHandle].WritePos = 0;
	ListInit(&My_QueuePool[*QueueHandle].ReaderSleepList);
	ListInit(&My_QueuePool[*QueueHandle].WriterSleepList);
	My_QueuePool[*QueueHandle].IsUsed = MY_QUEUE_USED;

MY_QUEUE_CREATE_EXIT:
	MY_EXIT_CRITICAL();
	return Ret;
}

static void My_QueueSleep(My_TCB *Tcb,myList *SleepListHead,My_BlockType BlockType)
{
	My_TaskReadyToBlock(Tcb, SleepListHead, BlockType, MY_BLOCK_SORT_PRIO);
}
static void My_QueueWakeup(myList *SleepListHead)
{
	My_TCB *WakeUpTaskTcb = MY_NULL;
	WakeUpTaskTcb = ListFirstEntry(SleepListHead, My_TCB, EventSleepList);
	My_TaskBlockToReady(WakeUpTaskTcb);
}

static my_uint32 __My_Queue_Write(my_uint32 QueueHandle, const void *Buffer, 
									my_uint32 Size,My_BlockType BlockType, my_uint32 Timeout)
{
	my_uint32 Ret = MY_SUCCESS;
	My_Queue* Queue = MY_NULL;
	My_TCB * TaskTcb = CurrentTCB;
	my_uint32 index = 0;
	my_uint8* BufferAddr = 0;
	MY_CHECK_NULL_POINTER(Buffer);
	My_Queue_Check_Handle_Valid(QueueHandle);
	My_Queue_Check_Handle_Created(QueueHandle);

	MY_ENTER_CRITICAL();
	Queue = MY_QUEUE_HANDLE_TO_POINTER(QueueHandle);
	if (Size > Queue->ElementSingleSize)
	{
		Ret = MY_QUEUE_WR_DATA_TOO_BIG;
		goto __MY_QUEUE_WRITE_EXIT;
	}

	while(My_QueueFull(QueueHandle))
	{
		if (Timeout == 0)
		{
			Ret = MY_QUEUE_TRY_WR_FAILED;
			goto __MY_QUEUE_WRITE_EXIT;
		}
		if (ATCH_IsInterruptContext())
		{
			Ret = MY_QUEUE_WR_FULL_IN_INTR_CONTEXT;
			goto __MY_QUEUE_WRITE_EXIT;
		}
		if (My_IsSchedulerSuspending())
		{
			Ret = MY_QUEUE_WR_FULL_IN_SCH_SUSPEND;
			goto __MY_QUEUE_WRITE_EXIT;
		}

		TaskTcb->WakeUpTime = My_GetCurrentTime() + Timeout;
		TaskTcb->EventTimeoutWakeup = MY_EVENT_NO_TIMEOUT;
		My_QueueSleep(TaskTcb, &Queue->WriterSleepList, BlockType);
		My_Schedule();
		MY_EXIT_CRITICAL();

		MY_ENTER_CRITICAL();
		if (TaskTcb->EventTimeoutWakeup == MY_EVENT_WAIT_TIMEOUT)
		{
			Ret = MY_QUEUE_WR_WAIT_TIMEOUT;
			goto __MY_QUEUE_WRITE_EXIT;			
		}
	}
	index = Queue->WritePos;
	BufferAddr = MY_QUEUE_INDEX_TO_BUFFERADDR(Queue, index);
	My_MemCopy(BufferAddr, (void*)Buffer, Size);
	My_QueueWritePosIncrease(Queue);

	if (!ListIsEmpty(&Queue->ReaderSleepList))
	{
		My_QueueWakeup(&Queue->ReaderSleepList);
		My_Schedule();
	}

__MY_QUEUE_WRITE_EXIT:
	MY_EXIT_CRITICAL();
	return Ret;
}

my_uint32 My_QueueWrite(my_uint32 QueueHandle, const void *Buffer, my_uint32 Size)
{
	return __My_Queue_Write(QueueHandle, Buffer, Size, MY_BLOCK_TYPE_ENDLESS, 0xFF);
}

my_uint32 My_QueueTryWrite(my_uint32 QueueHandle, const void *Buffer, my_uint32 Size)
{
	return __My_Queue_Write(QueueHandle, Buffer, Size, MY_BLOCK_TYPE_ENDLESS, 0x00);
}
my_uint32 My_QueueWriteTimeout(my_uint32 QueueHandle, const void *Buffer, my_uint32 Size, my_uint32 Timeout)
{
	return __My_Queue_Write(QueueHandle, Buffer, Size, MY_BLOCK_TYPE_TIMEOUT, Timeout);
}

static my_uint32 __My_QueueRead(my_uint32 QueueHandle, void *Buffer, my_uint32 Size,My_BlockType BlockType,my_uint32 Timeout)
{
	my_uint32 Ret = MY_SUCCESS;
	My_Queue * Queue = MY_NULL;
	My_TCB * TaskTcb = CurrentTCB;
	my_uint32 index = 0;
	my_uint8 * BufferAddr = MY_NULL;

	MY_CHECK_NULL_POINTER(Buffer);
	My_Queue_Check_Handle_Valid(QueueHandle);
	My_Queue_Check_Handle_Created(QueueHandle);

	MY_ENTER_CRITICAL();
	Queue = MY_QUEUE_HANDLE_TO_POINTER(QueueHandle);
	if (Size > Queue->ElementSingleSize)
	{
		Ret = MY_QUEUE_WR_DATA_TOO_BIG;
		goto __MY_QUEUE_READ_EXIT;
	}
	while (My_QueueEmpty(QueueHandle))
	{
		if (Timeout == 0)
		{
			Ret = MY_QUEUE_TRY_RD_FAILED;
			goto __MY_QUEUE_READ_EXIT;
		}
		if (ATCH_IsInterruptContext())
		{
			Ret = MY_QUEUE_RD_EMPTY_IN_INTR_CONTEXT;
			goto __MY_QUEUE_READ_EXIT;
		}
		if (My_IsSchedulerSuspending())
		{
			Ret = MY_QUEUE_RD_EMPTY_IN_SCH_SUSPEND;
			goto __MY_QUEUE_READ_EXIT;
		}

		TaskTcb->EventTimeoutWakeup = MY_EVENT_NO_TIMEOUT;
		TaskTcb->WakeUpTime = My_GetCurrentTime() + Timeout;
		My_QueueSleep(TaskTcb, &Queue->ReaderSleepList, BlockType);
		My_Schedule();
		MY_EXIT_CRITICAL();


		MY_ENTER_CRITICAL();
		if (TaskTcb->EventTimeoutWakeup == MY_EVENT_WAIT_TIMEOUT)
		{
			Ret = MY_QUEUE_RD_WAIT_TIMEOUT;
			goto __MY_QUEUE_READ_EXIT;
		}
	}
	index = Queue->ReadPos;
	BufferAddr = MY_QUEUE_INDEX_TO_BUFFERADDR(Queue, index);
	My_MemCopy(Buffer, BufferAddr, Size);
	My_QueueReadPosIncrease(Queue);
	if (!ListIsEmpty(&Queue->WriterSleepList))
	{
		My_QueueWakeup(&Queue->WriterSleepList);
		My_Schedule();
	}
__MY_QUEUE_READ_EXIT:
	MY_EXIT_CRITICAL();
	return Ret;

}
my_uint32 My_QueueRead(my_uint32 QueueHandle, void *Buffer, my_uint32 Size)
{
	return __My_QueueRead(QueueHandle, Buffer, Size, MY_BLOCK_TYPE_ENDLESS, 0x01);
}

my_uint32 My_QueueTryRead(my_uint32 QueueHandle, void *Buffer, my_uint32 Size)
{
	return __My_QueueRead(QueueHandle, Buffer, Size, MY_BLOCK_TYPE_ENDLESS, 0x00);
}

my_uint32 My_QueueReadTimeout(my_uint32 QueueHandle, void *Buffer, my_uint32 Size, my_uint32 Timeout)
{

	return __My_QueueRead(QueueHandle, Buffer, Size, MY_BLOCK_TYPE_TIMEOUT,Timeout);
}

my_uint32 My_QueueDestroy(my_uint32 QueueHandle)
{
	My_Queue *Queue = MY_NULL;
	my_uint32 Ret = MY_SUCCESS;
	My_Queue_Check_Handle_Created(QueueHandle);
	My_Queue_Check_Handle_Valid(QueueHandle);
	MY_ENTER_CRITICAL();
	Queue = MY_QUEUE_HANDLE_TO_POINTER(QueueHandle);
	if (!ListIsEmpty(&Queue->ReaderSleepList))
	{
		Ret = MY_QUEUE_DESTORY_RD_SLP;
		goto MY_QUEUE_DESTROY_EXIT;
	}
	if (!ListIsEmpty(&Queue->WriterSleepList))
	{
		Ret = MY_QUEUE_DESTORY_WR_SLP;
		goto MY_QUEUE_DESTROY_EXIT;
	}
	if (!My_QueueEmpty(QueueHandle))
	{
		Ret = MY_QUEUE_DESTORY_QUEUE_NOT_EMPTY;
		goto MY_QUEUE_DESTROY_EXIT;
	}
	My_Free(Queue->DataBuffer);
	Queue->IsUsed = MY_QUEUE_UNUSED;

MY_QUEUE_DESTROY_EXIT:
	MY_EXIT_CRITICAL();
	return Ret;
}
my_uint32 My_QueueRemainingSpace(my_uint32 QueueHandle)
{
	My_Queue *Queue = MY_QUEUE_HANDLE_TO_POINTER(QueueHandle);
	my_uint32 UsedSize = 0;
	if (Queue->ReadPos > Queue->WritePos)
	{
		UsedSize = Queue->ReadPos - Queue->WritePos;
	}
	else
	{
		UsedSize = Queue->WritePos - Queue->ReadPos;
	}
	return Queue->ElementNum - UsedSize;
}

my_uint8 My_QueueEmpty(my_uint32 QueueHandle)
{
	My_Queue *Queue = MY_QUEUE_HANDLE_TO_POINTER(QueueHandle);
	return Queue->ReadPos == Queue->WritePos;
}
my_uint8 My_QueueFull(my_uint32 QueueHandle)
{
	return My_QueueRemainingSpace(QueueHandle) == 0;
}
