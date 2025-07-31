#include "my_timer.h"
#include "my_config.h"
#include "my_error_code.h"
#include "my_time.h"
#include "my_scheduler.h"
#include "my_printk.h"
#include "my_task.h"
#include "my_lib.h"
#include "my_critical.h"

My_TimerManager TimerManager;
My_Timer My_TimerPool[MY_TIMER_MAX_NUM];
static my_uint32 My_TimerTaskHandle;

#define MY_TIMER_HANDLE_TO_POINTER(Handle) &My_TimerPool[Handle]
#define MY_TIMER_CHECK_HANDLE_VALID(Handle)\
{\
	if (Handle >= MY_TIMER_MAX_NUM)\
		return MY_TIMER_HANDLE_INVALID;\
}

#define MY_TIMER_CHECK_BEEN_CREATED(Handle)\
{\
	if(My_TimerPool[Handle].Used==MY_TIMER_UNUSED)\
		return MY_TIMER_NOT_BEEN_CREATED;\
}


void My_TimerInit(void)
{
	my_uint32 i;
	for (i = 0; i < MY_TIMER_MAX_NUM; i++)
	{
		My_TimerPool[i].Used = MY_TIMER_UNUSED;
	}
	TimerManager.NextWakeupTime = 0;
	ListInit(&TimerManager.TimersList);
}

static my_uint32 My_GetTimerResource(my_uint32 *TimerHandle)
{
	my_uint32 i;
	for (i = 0; i < MY_TIMER_MAX_NUM; i++)
	{
		if (My_TimerPool[i].Used == MY_TIMER_UNUSED)
			break;
	}
	if (i >= MY_TIMER_MAX_NUM)
		return MY_NOT_ENOUGH_TMR_RESOURCE;
	*TimerHandle = i;
	return MY_SUCCESS;
}
my_uint32 My_TimerCreate(my_uint32* TimerHandle, My_TimerMode Mode, my_uint32 Interval, My_TimerHandler TimeoutHandler, void* FuncParamer)
{
	my_uint32 Ret = MY_SUCCESS;

	MY_CHECK_NULL_POINTER(TimerHandle);
	MY_CHECK_NULL_POINTER(TimeoutHandler);
	if (Interval >= MY_TSK_DLY_MAX)
	{
		return MY_TMR_INVALID_INTERVAL;
	}
	MY_ENTER_CRITICAL();
	Ret = My_GetTimerResource(TimerHandle);
	if (Ret != MY_SUCCESS)
	{
		goto MY_TIMER_CREATE_EXIT;
	}
	My_TimerPool[*TimerHandle].Interval = Interval;
	My_TimerPool[*TimerHandle].Mode = Mode;
	My_TimerPool[*TimerHandle].Param = FuncParamer;
	My_TimerPool[*TimerHandle].Handler = TimeoutHandler;
	My_TimerPool[*TimerHandle].WakeupTime = 0;
	My_TimerPool[*TimerHandle].Status = MY_TIMER_STOP;
	ListInit(&My_TimerPool[*TimerHandle].List);
	My_TimerPool[*TimerHandle].Used = MY_TIMER_USED;
MY_TIMER_CREATE_EXIT:
	MY_EXIT_CRITICAL();
	return Ret;
}
static void My_AddTimerToManager(My_Timer* Timer)
{
	myList * TargetList = &TimerManager.TimersList;
	myList * ListIterator = MY_NULL;
	My_Timer* TimerIterator = MY_NULL;
	if (ListIsEmpty(TargetList))
	{
		ListAddAfter(&Timer->List, TargetList);
	}
	else
	{
		ListForEach(ListIterator, TargetList)
		{
			TimerIterator = ListEntry(ListIterator, My_Timer, List);
			if (TimerIterator->Interval > Timer->Interval)
				break;
		}
		if (ListIterator == TargetList)
		{
			ListAddBefore(&Timer->List, TargetList);
		}
		else
		{
			ListAddBefore(&Timer->List, ListIterator);
		}
	}
	TimerIterator = ListFirstEntry(&TimerManager.TimersList, My_Timer, List);
	TimerManager.NextWakeupTime = TimerIterator->WakeupTime;
}
my_uint32 My_TimerNextWakeupTime()
{
	return TimerManager.NextWakeupTime;
}
my_uint32 My_TimerStart(my_uint32 TimerHandle)
{
	my_uint32 Ret = MY_SUCCESS;
	My_Timer *Timer =  MY_NULL;
	MY_TIMER_CHECK_HANDLE_VALID(TimerHandle);
	MY_TIMER_CHECK_BEEN_CREATED(TimerHandle);
	MY_ENTER_CRITICAL();
	Timer = MY_TIMER_HANDLE_TO_POINTER(TimerHandle);
	if (Timer->Status == MY_TIMER_RUNNING)
	{
		Ret = MY_TIMER_ALREADY_RUNNING;
		MY_EXIT_CRITICAL();
		return Ret;
	}
	Timer->Status = MY_TIMER_RUNNING;
	Timer->WakeupTime = My_GetCurrentTime() + Timer->Interval;
	My_AddTimerToManager(Timer);
	MY_EXIT_CRITICAL();
	return Ret;
}
my_uint32 My_TimerStop(my_uint32 TimerHandle)
{
	my_uint32 Ret = MY_SUCCESS;
	My_Timer* Timer = MY_NULL;

	MY_TIMER_CHECK_HANDLE_VALID(TimerHandle);
	MY_TIMER_CHECK_BEEN_CREATED(TimerHandle);
	MY_ENTER_CRITICAL();
	Timer = MY_TIMER_HANDLE_TO_POINTER(TimerHandle);
	if (Timer->Status == MY_TIMER_STOP)
	{
		Ret = MY_TIMER_NOT_RUNNING;
		goto MY_TIMER_STOP_EXIT;
	}
	Timer->Status = MY_TIMER_STOP;
	ListDelete(&Timer->List);

MY_TIMER_STOP_EXIT:
	MY_EXIT_CRITICAL();
	return Ret;
}
my_uint32 My_TimerDelete(my_uint32 TimerHandle)
{
	my_uint32 Ret = MY_SUCCESS;
	My_Timer *Timer = MY_NULL;

	MY_TIMER_CHECK_HANDLE_VALID(TimerHandle);
	MY_TIMER_CHECK_BEEN_CREATED(TimerHandle);
	MY_ENTER_CRITICAL();
	Timer = MY_TIMER_HANDLE_TO_POINTER(TimerHandle);
	if (Timer->Status == MY_TIMER_RUNNING)
	{
		ListDelete(&Timer->List);
		Timer->Status = MY_TIMER_STOP;
	}
	Timer->Used = MY_TIMER_UNUSED;
	MY_EXIT_CRITICAL();
	return Ret;
}

void My_TimerTaskEntry(void *Parameter)
{
	my_uint32 CurrentTime = 0;
	My_Timer *Timer;
	My_TCB * TimerTaskTcb = MY_TSK_HANDLE_TO_TCB(My_TimerTaskHandle);
	while (1)
	{
		MY_ENTER_CRITICAL();
		while (!ListIsEmpty(&TimerManager.TimersList))
		{
			CurrentTime = My_GetCurrentTime();
			Timer = ListFirstEntry(&TimerManager.TimersList, My_Timer, List);
			if (MY_TIME_AFTER_EQ(CurrentTime, Timer->WakeupTime))
			{
				Timer->Handler(Timer->Param);
				ListDelete(&Timer->List);
				if (Timer->Mode == MY_TIMER_AUTO_RELOAD)
				{
					Timer->WakeupTime = My_GetCurrentTime() + Timer->Interval;
					My_AddTimerToManager(Timer);
				}
				else
				{
					Timer->Status = MY_TIMER_STOP;
				}
			}
			else
			{
				break;
			}
		}
		if (!ListIsEmpty(&TimerManager.TimersList))
		{
			Timer = ListFirstEntry(&TimerManager.TimersList, My_Timer, List);
			TimerManager.NextWakeupTime = Timer->WakeupTime;
		}
		MY_EXIT_CRITICAL();
		My_TaskReadyToSuspend(TimerTaskTcb);
		My_Schedule();
	}
}

void My_TimerCheck(my_uint32 CurrentTime)
{
	My_TCB *TimerTaskTcb = MY_TSK_HANDLE_TO_TCB(My_TimerTaskHandle);
	if (!ListIsEmpty(&TimerManager.TimersList))
	{
		if (MY_TIME_AFTER_EQ(CurrentTime, TimerManager.NextWakeupTime))
		{
			if (TimerTaskTcb->State == MY_TASK_SUSPEND)
			{
				My_TaskSuspendToReady(TimerTaskTcb);
			}
		}
	}
}

void My_TimerTaskCreate(void)
{
	My_TCBInitParameter Param;
	My_MemCopy(Param.Name, "TimerTask",MY_CONFIG_TASK_NAME_LEN);
	Param.Priority = MY_TIMER_TASK_PRIORITY;
	Param.StackSize = MY_TIMER_TASK_STACK_SIZE;
	Param.TaskEntry = My_TimerTaskEntry;
	My_TaskCreate(&Param, &My_TimerTaskHandle);
}
