#include "my_sem.h"
#include "my_config.h"
#include "my_task.h"
#include "my_scheduler.h"
#include "my_error_code.h"
#include "my_critical.h"
#include "my_time.h"
#include "my_arch.h"
static My_Sem My_SemPool[MY_SEM_MAX_NUM];

extern My_TCB * volatile CurrentTCB;

#define MY_SEM_HANDLE_TO_POINTER(Handle) &My_SemPool[Handle]
#define MY_SEM_CHECK_HANDLE_VALID(Handle)\
{\
	if (Handle >= MY_SEM_MAX_NUM)\
		return MY_SEM_HANDLE_INVALID;\
}

#define MY_SEM_CHECK_HANDLE_CREATED(Handle)\
{\
	if (My_SemPool[Handle].Used == MY_SEM_UNUSED)\
		return MY_SEM_NOT_BEEN_CREATED;\
}
void My_SemInit(void)
{
	my_uint32 i;
	for (i = 0; i < MY_SEM_MAX_NUM; i++)
	{
		My_SemPool[i].Count = 0;
		My_SemPool[i].Used = MY_SEM_UNUSED;
		ListInit(&My_SemPool[i].List);
	}
}

static my_uint32 My_GetSemResource(my_uint32 *SemHandle)
{
	my_uint32 i;
	for (i = 0; i < MY_SEM_MAX_NUM; i++)
	{
		if (My_SemPool[i].Used == MY_SEM_UNUSED)
			break;
	}
	if (i == MY_MUTEX_MAX_NUM)
		return MY_NOT_ENOUGH_MUTEX_RESOURCE;
	*SemHandle = i;
	return MY_SUCCESS;
}

my_uint32 My_SemCreate(my_uint32 *SemHandle,my_uint32 Count)
{

	my_uint32 Ret = MY_SUCCESS;
	MY_CHECK_NULL_POINTER(SemHandle);
	if (Count > MY_SEM_COUNT_MAX)
	{
		return MY_SEM_OVERFLOW;
	}
	MY_ENTER_CRITICAL();
	
	Ret = My_GetSemResource(SemHandle);
	if (Ret != MY_SUCCESS)
	{
		goto MY_SEM_CREATE_EXIT;
	}
	My_SemPool[*SemHandle].Count = Count;
	My_SemPool[*SemHandle].Used = MY_SEM_USED;
	ListInit(&My_SemPool[*SemHandle].List);

MY_SEM_CREATE_EXIT:
	MY_EXIT_CRITICAL();
	return Ret;
}

static my_uint32 __My_SemGet(my_uint32 SemHandle, My_BlockType BlockType, my_uint32 Timeout)
{
	my_uint32 Ret = MY_SUCCESS;
	My_Sem *Sem = MY_NULL;
	My_TCB *TaskTcb = CurrentTCB;
	MY_SEM_CHECK_HANDLE_VALID(SemHandle);
	MY_SEM_CHECK_HANDLE_CREATED(SemHandle);

	MY_ENTER_CRITICAL();

	if (ATCH_IsInterruptContext())
	{
		Ret = MY_SEM_GET_IN_INTR_CONTEXT;
		goto MY_SEM_GET_EXIT;
	}
	if (My_IsSchedulerSuspending())
	{
		Ret = MY_SEM_GET_IN_SCH_SUSPEND;
		goto MY_SEM_GET_EXIT;
	}
	Sem = MY_SEM_HANDLE_TO_POINTER(SemHandle);
	if (Sem->Count > 0)
	{
		Sem->Count--;
		Ret = MY_SUCCESS;
		goto MY_SEM_GET_EXIT;
	}
	if (Timeout == 0)
	{
		Ret = MY_SEM_TRY_GET_FAILED;
		goto MY_SEM_GET_EXIT;
	}
	TaskTcb->EventTimeoutWakeup = MY_EVENT_NO_TIMEOUT;
	TaskTcb->WakeUpTime = My_GetCurrentTime() + Timeout;
	My_TaskReadyToBlock(TaskTcb, &Sem->List, BlockType, MY_BLOCK_SORT_PRIO);
	My_Schedule();
	MY_EXIT_CRITICAL();
	
	MY_ENTER_CRITICAL();
	if (TaskTcb->EventTimeoutWakeup == MY_EVENT_WAIT_TIMEOUT)
	{
		Ret = MY_SEM_GET_TIMEOUT;
	}
MY_SEM_GET_EXIT:
	MY_EXIT_CRITICAL();
	return Ret;
}

my_uint32 My_SemGet(my_uint32 SemHandle)
{
	return __My_SemGet(SemHandle, MY_BLOCK_TYPE_ENDLESS, 0x01);
}

my_uint32 My_SemTryGet(my_uint32 SemHandle)
{
	return __My_SemGet(SemHandle, MY_BLOCK_TYPE_TIMEOUT, 0x00);
}

my_uint32 My_SemGetTimeout(my_uint32 SemHandle, int Timeout)
{
	return __My_SemGet(SemHandle, MY_BLOCK_TYPE_TIMEOUT, Timeout);
}

my_uint32 My_SemRelease(my_uint32 SemHandle)
{
	my_uint32 Ret = MY_SUCCESS;
	My_Sem *Sem = MY_NULL;
	My_TCB *WakeupTcb = MY_NULL;

	MY_SEM_CHECK_HANDLE_VALID(SemHandle);
	MY_SEM_CHECK_HANDLE_CREATED(SemHandle);
	
	MY_ENTER_CRITICAL();
	Sem = MY_SEM_HANDLE_TO_POINTER(SemHandle);
	if (Sem->Count == MY_SEM_COUNT_MAX)
	{
		Ret = MY_SEM_OVERFLOW;
		goto MY_SEM_RELEASE_EXIT;
	}
	if (!ListIsEmpty(&Sem->List))
	{
		WakeupTcb = ListFirstEntry(&Sem->List, My_TCB, EventSleepList);
		My_TaskBlockToReady(WakeupTcb);
		My_Schedule();
	}
	else
	{
		Sem->Count--;
	}
MY_SEM_RELEASE_EXIT:
	MY_EXIT_CRITICAL();
	return Ret;
}

my_uint32 My_SemDestroy(my_uint32 SemHandle)
{
	my_uint32 Ret = MY_SUCCESS;
	My_Sem *Sem = MY_NULL;
	My_TCB *WakeupTcb = MY_NULL;
	MY_SEM_CHECK_HANDLE_VALID(SemHandle);
	MY_SEM_CHECK_HANDLE_CREATED(SemHandle);
	MY_ENTER_CRITICAL();
	Sem = MY_SEM_HANDLE_TO_POINTER(SemHandle);
	while(!ListIsEmpty(&Sem->List))
	{
		WakeupTcb = ListFirstEntry(&Sem->List, My_TCB, EventSleepList);
		My_TaskBlockToReady(WakeupTcb);
	}
	Sem->Count = 0;
	Sem->Used = MY_SEM_UNUSED;
	My_Schedule();
	MY_EXIT_CRITICAL();
	return Ret;
}
