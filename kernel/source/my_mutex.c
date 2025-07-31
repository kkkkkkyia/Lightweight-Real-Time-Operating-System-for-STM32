#include  "my_mutex.h"
#include  "my_config.h"
#include "my_error_code.h"
#include "my_critical.h"
#include "my_types.h"
#include "my_scheduler.h"
#include "my_arch.h"
#include "my_time.h"
#include "my_printk.h"

My_Mutex My_MutexPool[MY_MUTEX_MAX_NUM];
extern My_TCB * volatile CurrentTCB ;
#define MY_MUTEX_HANDLE_TO_POINTER(Handle) (&My_MutexPool[Handle])

#define MY_MUTEX_CHECK_HANDLE_VALID(MutexHandle)\
{\
	if (MutexHandle >= MY_MUTEX_MAX_NUM)\
		return MY_MUTEX_HANDLE_INVALID;\
}
#define MY_MUTEX_CHECK_BEEN_CREATED(MutexHandle)\
{\
	if (My_MutexPool[MutexHandle].Used == MY_MUTEX_UNUSED)\
		return MY_MUTEX_NOT_BEEN_CREATED;\
}

void My_MutexInit(void)
{
	my_uint32 i;
	for (i = 0; i < MY_MUTEX_MAX_NUM; i++)
	{
		My_MutexPool[i].Owner =MY_NULL;
		My_MutexPool[i].OwnerHoldCount = 0;
		My_MutexPool[i].OwnerPriority = 0;
		My_MutexPool[i].Used = MY_MUTEX_UNUSED;
		ListInit(&My_MutexPool[i].SleepList);
	}
}
static my_uint32 My_GetMutexResource(my_uint32 *MutexHandle)
{
	my_uint32 i;
	for (i = 0; i < MY_MUTEX_MAX_NUM; i++)
	{
		if (My_MutexPool[i].Used == MY_MUTEX_UNUSED)
		{
			break;
		}
	}
	if (i == MY_MUTEX_MAX_NUM)
		return MY_NOT_ENOUGH_MUTEX_RESOURCE;
	*MutexHandle = i;
	return MY_SUCCESS;
}
my_uint32 My_MutexCreate(my_uint32 *MutexHandle)
{
	my_uint32 Ret = MY_SUCCESS;
	MY_CHECK_NULL_POINTER(MutexHandle);
	MY_ENTER_CRITICAL();
	Ret = My_GetMutexResource(MutexHandle);
	if (Ret != MY_SUCCESS)
		goto MY_MUTEX_CREATE_EXIT;
	My_MutexPool[*MutexHandle].OwnerHoldCount = 0;
	My_MutexPool[*MutexHandle].Owner = MY_NULL;
	My_MutexPool[*MutexHandle].OwnerPriority = 0;
	ListInit(&My_MutexPool[*MutexHandle].SleepList);
	My_MutexPool[*MutexHandle].Used = MY_MUTEX_USED;

MY_MUTEX_CREATE_EXIT:
	MY_EXIT_CRITICAL();
	return Ret;
}
static void  My_MutexSleep(My_TCB *TaskTcb, My_Mutex *Mutex, My_BlockType BlockType)
{
	if (TaskTcb->Priority < Mutex->OwnerPriority)
	{
		My_TaskChangePriorityTemp(Mutex->Owner, TaskTcb->Priority);
		Mutex->OwnerPriority = TaskTcb->Priority;
	}
	My_TaskReadyToBlock(TaskTcb, &Mutex->SleepList, BlockType, MY_BLOCK_SORT_PRIO);
}
static my_uint32 __My_MutexLock(my_uint32 MutexHandle, My_BlockType BlockType, my_uint32 Timeout)
{
	my_uint32 Ret = MY_SUCCESS;
	My_Mutex * Mutex = MY_NULL;
	My_TCB *TaskTcb = CurrentTCB;
	MY_MUTEX_CHECK_HANDLE_VALID(MutexHandle);
	MY_MUTEX_CHECK_BEEN_CREATED(MutexHandle);

	MY_ENTER_CRITICAL();
	if (ATCH_IsInterruptContext())
	{
		Ret = MY_USE_MUTEX_IN_INTR_CONTEXT;
		goto __MY_MUTEX_LOCK_EXIT;
	}
	if (My_IsSchedulerSuspending())
	{
		Ret = MY_USE_MUTEX_IN_SCH_SUSPEND;
		goto __MY_MUTEX_LOCK_EXIT;
	}

	Mutex = MY_MUTEX_HANDLE_TO_POINTER(MutexHandle);
	if (Mutex->OwnerHoldCount == 0)
	{
		Mutex->Owner = TaskTcb;
		Mutex->OwnerHoldCount = 1;
		Mutex->OwnerPriority = TaskTcb->Priority;
		DEBUG_TRACE("Mutex Locked by:%x\n",(my_uint32)TaskTcb);
		goto __MY_MUTEX_LOCK_EXIT;
	}
	if (Mutex->Owner == TaskTcb)
	{
		Mutex->OwnerHoldCount++;
		goto __MY_MUTEX_LOCK_EXIT;
	}
	if (Timeout == 0)
	{
		Ret = MY_TRY_MUTEX_LOCK_FAILED;
		goto __MY_MUTEX_LOCK_EXIT;
	}

	TaskTcb->EventTimeoutWakeup = MY_EVENT_NO_TIMEOUT;
	TaskTcb->WakeUpTime = My_GetCurrentTime() + Timeout;
	My_MutexSleep(TaskTcb,Mutex,BlockType);
	DEBUG_TRACE("Mutex Lock Failed and Blocked:%x\n",(my_uint32)TaskTcb);

	My_Schedule();

	MY_EXIT_CRITICAL();
	MY_ENTER_CRITICAL();

	if (TaskTcb->EventTimeoutWakeup == MY_EVENT_WAIT_TIMEOUT)
	{
		Ret = MY_MUTEX_WAIT_TIMEOUT;
	}

__MY_MUTEX_LOCK_EXIT:
	MY_EXIT_CRITICAL();
	return Ret;
}
my_uint32 My_MutexLock(my_uint32 MutexHandle)
{
	return __My_MutexLock(MutexHandle, MY_BLOCK_TYPE_ENDLESS, 0x01);
}
my_uint32 My_MutexLockTimeout(my_uint32 MutexHandle, my_uint32 Timeout)
{
	return __My_MutexLock(MutexHandle, MY_BLOCK_TYPE_TIMEOUT, Timeout);

}
my_uint32 My_MutexTryLock(my_uint32 MutexHandle)
{
	return __My_MutexLock(MutexHandle, MY_BLOCK_TYPE_TIMEOUT, 0x00);
}

static my_uint8 MY_Mutex_Wakeup(My_TCB *TaskTcb,My_Mutex *Mutex)
{
	my_uint8 Ret_NeedReschedule = 0;
	My_TCB *WakeupTcb;
	if (TaskTcb->BasePriority != Mutex->OwnerPriority)
	{
		My_TaskResumePriority(TaskTcb);
	}
	if (!ListIsEmpty(&Mutex->SleepList))
	{
		Ret_NeedReschedule = 1;
		WakeupTcb = ListFirstEntry(&Mutex->SleepList, My_TCB, EventSleepList);
		My_TaskBlockToReady(WakeupTcb);
		Mutex->Owner = WakeupTcb;
		Mutex->OwnerPriority = WakeupTcb->Priority;
		Mutex->OwnerHoldCount = 1;
	}
	else
	{
		Mutex->OwnerPriority = MY_MIN_PRIORITY;
		Mutex->Owner = MY_NULL;
	}
	return Ret_NeedReschedule;
}
my_uint32 My_MutexUnlock(my_uint32 MutexHandle)
{
	my_uint32 Ret = MY_SUCCESS;
	My_TCB *TaskTcb = CurrentTCB;
	My_Mutex *Mutex = MY_NULL;
	MY_MUTEX_CHECK_HANDLE_VALID(MutexHandle);
	MY_MUTEX_CHECK_BEEN_CREATED(MutexHandle);
	MY_ENTER_CRITICAL();
	if (ATCH_IsInterruptContext())
	{
		Ret = MY_USE_MUTEX_IN_INTR_CONTEXT;
		goto __MY_MUTEX_UNLOCK;
	}
	if (My_IsSchedulerSuspending())
	{
		Ret = MY_USE_MUTEX_IN_SCH_SUSPEND;
		goto __MY_MUTEX_UNLOCK;
	}
	Mutex = MY_MUTEX_HANDLE_TO_POINTER(MutexHandle);
	if (Mutex->OwnerHoldCount == 0)
	{
		Ret = MY_MUTEX_UNLOCK_INVALID;
		MY_PRINTK_ERROR("MutexUNlock without lock\n");
		goto __MY_MUTEX_UNLOCK;
	}
	if (Mutex->Owner != TaskTcb)
	{
		Ret = MY_MUTEX_UNLOCK_NOT_OWNER;
		MY_PRINTK_ERROR("Mutex unlock Owner error\n");
		goto __MY_MUTEX_UNLOCK;
	}
	Mutex->OwnerHoldCount--;
	if (Mutex->OwnerHoldCount != 0)
	{
		Ret = MY_SUCCESS;
		goto __MY_MUTEX_UNLOCK;
	}
	DEBUG_TRACE("Mutex Unlocked\n");
	if (MY_Mutex_Wakeup(TaskTcb,Mutex))
	{
		DEBUG_TRACE("Mutex Unlocked and Wake Task\n");
		My_Schedule();
	}
__MY_MUTEX_UNLOCK:
	MY_EXIT_CRITICAL();
	return Ret;
}
my_uint32 My_MutexDestroy(my_uint32 MutexHandle)
{
	my_uint32 Ret = MY_SUCCESS;
	My_Mutex *Mutex = MY_NULL;
	MY_MUTEX_CHECK_HANDLE_VALID(MutexHandle);
	MY_MUTEX_CHECK_BEEN_CREATED(MutexHandle);
	MY_ENTER_CRITICAL();
	Mutex = MY_MUTEX_HANDLE_TO_POINTER(MutexHandle);
	if (!ListIsEmpty(&Mutex->SleepList))
	{
		Ret = MY_MUTEX_DESTORY_IN_NO_EMPTY;
		goto MY_MUTEX_DESTROY_EXIT;
	}
	if (Mutex->OwnerHoldCount == 0)
	{
		Ret = MY_MUTEX_DESTORY_IN_OWNER_USING;
		goto MY_MUTEX_DESTROY_EXIT;
	}

	Mutex->Owner = MY_NULL;
	Mutex->Used = MY_MUTEX_UNUSED;
	Mutex->OwnerPriority = MY_MIN_PRIORITY;

MY_MUTEX_DESTROY_EXIT:
	MY_EXIT_CRITICAL();
	return Ret;
}

