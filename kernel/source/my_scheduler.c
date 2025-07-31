#include "my_scheduler.h"
#include "my_list.h"
#include "my_config.h"
#include "my_critical.h"
#include "my_time.h"


#if MY_TIMER_USE
	#include "my_timer.h"
#endif

My_TaskScheduler Scheduler;
my_uint8 TaskYield = 0;

extern My_TCB * volatile CurrentTCB ;
extern My_TCB * volatile SwitchNextTCB;

extern void My_ContextSwitch(void);


void My_ClearPriorityActive(my_uint8 Priority)
{
	Scheduler.PriorityActive &= (~(0x01<<Priority)) ;
}

void My_SetPriorityActive(my_uint8 Priority)
{
	Scheduler.PriorityActive |= (0x01<<Priority);
}


void My_SchedulerSuspend(void)
{
	MY_ENTER_CRITICAL();
	Scheduler.SchedulerSuspendNesting++;
	MY_EXIT_CRITICAL();
}

void My_ScheduleResume(void)
{
	MY_ENTER_CRITICAL();

	Scheduler.SchedulerSuspendNesting--;
	if ((Scheduler.SchedulerSuspendNesting == 0) && (Scheduler.ReSchedulePending == 1))
	{
		Scheduler.ReSchedulePending = 0;
		My_Schedule();
	}
/*
	//Pending
	if (Scheduler.SchedulerSuspendNesting == 0 && Scheduler.ReSchedulePending == RESCH_PENDING)
	{
		while (ListIsEmpty(&Scheduler.PendingReadyList) != 0)
		{
			PendingTcb = ListFirstEntry(&Scheduler.PendingReadyList,My_TCB,StateList);
			
			My_AddTaskToReadyList((my_uint32)PendingTcb);
			if (PendingTcb->Priority < CurrentTCB->Priority)
				TaskYield = 1;
		Scheduler.ReSchedulePending = NO_RESCH_PENDING;
		}
	}
	if (TaskYield)
		{
			My_Schedule();
		}
*/
	MY_EXIT_CRITICAL();
}

/**
 *  Called  By My_SystemTickHandle,Which is Called By (xPortSysTickHandler)SysTick Interrupt Handle Function.
 *  
 */
void My_SystemTickHandle(void)
{
	
}

void My_Schedule(void)
{
	my_uint8 NeedReSchedule = 0;
	if (My_IsSchedulerSuspending())
	{   //当前调度器挂起
		Scheduler.ReSchedulePending = 1;
		return;
	}
	SwitchNextTCB = My_GetHighestPriorityTask();	
	if (SwitchNextTCB->Priority != CurrentTCB->Priority)
	{
		NeedReSchedule = 1;
		goto MY_SCHEDULE_RIGHTNOW;
	}
	else if (SwitchNextTCB->Priority == CurrentTCB->Priority)
	{
		if (SwitchNextTCB != CurrentTCB)
		{
			NeedReSchedule = 1; 
			ListMoveTail(&SwitchNextTCB->StateList, &Scheduler.ReadyListHead[SwitchNextTCB->Priority]);
			goto MY_SCHEDULE_RIGHTNOW;
		}
		else
		{
			if (ListIsLast(&SwitchNextTCB->StateList, &Scheduler.ReadyListHead[SwitchNextTCB->Priority]))
			{
				NeedReSchedule = 0;
				goto MY_SCHEDULE_RIGHTNOW;
			}
			else
			{
				NeedReSchedule = 1;
				SwitchNextTCB = ListEntry(CurrentTCB->StateList.next, My_TCB, StateList);
				ListMoveTail(&CurrentTCB->StateList, &Scheduler.ReadyListHead[SwitchNextTCB->Priority]);
				ListMoveTail(&SwitchNextTCB->StateList, &Scheduler.ReadyListHead[SwitchNextTCB->Priority]);

			}
		}
	}
MY_SCHEDULE_RIGHTNOW:
	if (NeedReSchedule == 1)
	{
		My_ContextSwitch();
	}
}

my_uint8 My_IsSchedulerSuspending()
{
	my_uint8 isSuspending = 0;
	MY_ENTER_CRITICAL();
	isSuspending = Scheduler.SchedulerSuspendNesting ;
	
	
	MY_EXIT_CRITICAL();
	return isSuspending;
}

void My_ScheduleInit(void)
{
	for (int i = 0; i < MY_PRIORITY_NUM; i++)
	{
		ListInit(&Scheduler.ReadyListHead[i]);
	}
	ListInit(&Scheduler.BlockTimeoutListHead);
	ListInit(&Scheduler.SuspendListHead);
	ListInit(&Scheduler.DelayListHead);

	Scheduler.SchedulerSuspendNesting = 0;
	Scheduler.PriorityActive = 0;
	Scheduler.ReSchedulePending = NO_RESCH_PENDING;
	My_Schedule_Time_Slice = MY_SCHEDULE_TIME_SLICE_INIT_VALUE;
}
	
void My_StartScheduler(void)
{
	My_IdleTaskCreate();
	#if MY_TIMER_USE
	My_TimerTaskCreate();
	#endif
	CurrentTCB = My_GetHighestPriorityTask();
	xPortStartScheduler();
}


void My_AddTaskToReadyList(My_TCB * Tcb)
{
	ListAddAfter(&Tcb->StateList, &Scheduler.ReadyListHead[Tcb->Priority]);
	My_SetPriorityActive(Tcb->Priority);
	Tcb->State = MY_TASK_READY;
}
void My_AddTaskToEndlessBlockList(My_TCB * Tcb, myList *SleepHead, My_BlockSortType SortType)
{
	myList *ListIterator = MY_NULL;
	My_TCB *TcbIterator = MY_NULL;
	switch (SortType)
	{
	case MY_BLOCK_SORT_FIFO:
		ListAddAfter(&Tcb->EventSleepList, SleepHead);
		break;
	case MY_BLOCK_SORT_PRIO:
	{
		if (ListIsEmpty(SleepHead))
		{
			ListAddAfter(&Tcb->EventSleepList, SleepHead);
		}
		else
		{
			ListForEach(ListIterator, SleepHead)
			{
				TcbIterator = ListEntry(ListIterator, My_TCB, EventSleepList);
				if (TcbIterator->Priority < Tcb->Priority)
				{
					break;
				}
			}
			if (ListIterator == SleepHead)
			{
				ListAddAfter(&Tcb->EventSleepList, &TcbIterator->EventSleepList);
			}
			else
			{
				ListAddBefore(&Tcb->EventSleepList, &TcbIterator->EventSleepList);
			}
		}
	}
		break;
	}
	Tcb->State = MY_TASK_ENDLESS_BLOCKED;
}
void My_AddTaskToTimeSortList(My_TCB * Tcb, My_SchedulerStateList TargetList)
{
	myList *ListIterator = MY_NULL;
	My_TCB *TCB_Iterator = MY_NULL;
	myList *TargetListHead = MY_NULL;
	switch (TargetList)
	{
	case MY_DELAY_LIST:
	{
		TargetListHead = &Scheduler.DelayListHead;
	}
	break;

	case MY_BLOCKED_TIMEOUT_LIST:
	{
		TargetListHead = &Scheduler.BlockTimeoutListHead;
	}
	break;

	default:
	{
		TargetListHead = &Scheduler.DelayListHead;
	}
	break;
	}

	if (ListIsEmpty(TargetListHead))
	{
		ListAddAfter(&Tcb->StateList, TargetListHead);
	}
	else
	{
		// Insert in the delay list with sort
		ListForEach(ListIterator, TargetListHead)
		{
			TCB_Iterator = ListEntry(ListIterator, My_TCB, StateList);
			// If TCB_Iterator->WakeUpTime after TargetTCB->WakeUpTime
			if (MY_TIME_AFTER_EQ(TCB_Iterator->WakeUpTime, Tcb->WakeUpTime))
				break;
		}

		// Do not find position
		if (ListIterator == TargetListHead)
		{
			ListAddAfter(&Tcb->StateList, TargetListHead);
		}
		else
		{
			ListAddBefore(&Tcb->StateList, TCB_Iterator->StateList.pre);
		}
	}
}
void My_AddTaskToDelayList(My_TCB * Tcb)
{
	My_AddTaskToTimeSortList(Tcb, MY_DELAY_LIST);
	Tcb->State = MY_TASK_DELAY;
}
void My_AddTaskToTimeoutBlockedList(My_TCB * Tcb)
{
	My_AddTaskToTimeSortList(Tcb, MY_BLOCKED_TIMEOUT_LIST);
	Tcb->State = MY_TASK_TIMEOUT_BLOCKED;
}
void My_AddTaskToSuspendList(My_TCB * Tcb)
{
	ListAddAfter(&Tcb->StateList, &Scheduler.SuspendListHead);
	Tcb->State = MY_TASK_SUSPEND;
}
void My_RemoveTaskFromReadyList(My_TCB * Tcb)
{
	ListDelete(&Tcb->StateList);
	if (ListIsEmpty(&(Scheduler.ReadyListHead[Tcb->Priority])))
	{
		My_ClearPriorityActive(Tcb->Priority);
	}
	Tcb->State = MY_TASK_UNKNOWN;
}

void My_RemoveTaskFromSuspendList(My_TCB * Tcb)
{
	ListDelete(&Tcb->StateList);
	Tcb->State = MY_TASK_UNKNOWN;
}

void My_RemoveTaskFromBlockedList(My_TCB * Tcb)
{
	if (Tcb->State == MY_TASK_TIMEOUT_BLOCKED)
	{
		ListDelete(&Tcb->StateList);
	}
	ListDelete(&Tcb->EventSleepList);
	Tcb->State = MY_TASK_TIMEOUT_BLOCKED;
}
void My_RemoveTaskFromDelayList(My_TCB * Tcb)
{
	ListDelete(&Tcb->StateList);
	Tcb->State = MY_TASK_UNKNOWN;
}
void My_RemoveTaskFromUnknownList(My_TCB * Tcb)
{
	if ((Tcb->State == MY_TASK_ENDLESS_BLOCKED) || (Tcb->State == MY_TASK_TIMEOUT_BLOCKED))
	{
		ListDelete(&Tcb->EventSleepList);
	}

	if (Tcb->State != MY_TASK_ENDLESS_BLOCKED)
	{
		ListDelete(&Tcb->StateList);
	}

	if (Tcb->State == MY_TASK_READY)
	{
		if (ListIsEmpty(&Scheduler.ReadyListHead[Tcb->Priority]))
		{
			My_ClearPriorityActive(Tcb->Priority);
		}
	}
	Tcb->State = MY_TASK_UNKNOWN;
}


void My_TaskReadyToDelay(My_TCB * Tcb)
{
	My_RemoveTaskFromReadyList(Tcb);
	My_AddTaskToDelayList(Tcb);
}
void My_TaskDelayToReady(My_TCB * Tcb)
{
	My_RemoveTaskFromDelayList(Tcb);
	My_AddTaskToReadyList(Tcb);
}
void My_TaskReadyToSuspend(My_TCB * Tcb)
{
	My_RemoveTaskFromReadyList(Tcb);
	My_AddTaskToSuspendList(Tcb);
}
void My_TaskSuspendToReady(My_TCB * Tcb)
{
	My_RemoveTaskFromSuspendList(Tcb);
	My_AddTaskToReadyList(Tcb);
}
void My_TaskReadyToBlock(My_TCB * Tcb, myList* SleepHead,My_BlockType BlockType, My_BlockSortType SortType)
{
	My_RemoveTaskFromReadyList(Tcb);
	My_AddTaskToEndlessBlockList(Tcb,SleepHead,SortType);
	if (BlockType == MY_BLOCK_TYPE_TIMEOUT)
	{
		My_AddTaskToTimeoutBlockedList(Tcb);
	}
}

void My_TaskBlockToReady(My_TCB * Tcb)
{
	My_RemoveTaskFromBlockedList(Tcb);
	My_AddTaskToReadyList(Tcb);
}

void My_CheckDelayTaskWakeup(my_uint16 time)
{
	myList * ListIterator = MY_NULL;
	myList *ListIterator_prev = MY_NULL;
	My_TCB * TcbIterator = MY_NULL;

	if (ListIsEmpty(&Scheduler.DelayListHead))
	{
		return;
	}

	ListForEach(ListIterator, &Scheduler.DelayListHead)
	{
		TcbIterator = ListEntry(ListIterator, My_TCB, StateList);
		
		if (MY_TIME_AFTER_EQ(time, TcbIterator->WakeUpTime))
		{
			ListIterator_prev = ListIterator->pre;
			My_TaskDelayToReady(TcbIterator);
			ListIterator = ListIterator_prev;
		}
		else
			break;
	}
}

void My_CheckTaskBlockWakeup(my_uint32 time)
{
	myList * ListIterator = MY_NULL;
	myList *ListIterator_prev = MY_NULL;
	My_TCB * TcbIterator = MY_NULL;

	if (ListIsEmpty(&Scheduler.BlockTimeoutListHead))
	{
		return;
	}
	ListForEach(ListIterator, &Scheduler.BlockTimeoutListHead)
	{
		TcbIterator = ListEntry(ListIterator, My_TCB, StateList);
		if (MY_TIME_AFTER_EQ(time, TcbIterator->WakeUpTime))
		{
			ListIterator_prev = ListIterator->pre;
			TcbIterator->EventTimeoutWakeup = MY_EVENT_WAIT_TIMEOUT;
			My_TaskDelayToReady(TcbIterator);
			ListIterator = ListIterator_prev;
		}
		else
			break;
	}
}
void My_CheckTaskWakeup(my_uint32 time)
{
	My_CheckDelayTaskWakeup(time);
	My_CheckTaskBlockWakeup(time);
}

int My_Scheduler_IncrementTick(void)
{
  int Ret = pdFALSE;
	my_uint32 CurrentTime = 0;
	MY_ENTER_CRITICAL();
	if (My_IsSchedulerSuspending())
	{
		goto MY_SCHEDULER_INCREMENT_TICK_EXIT;
	}
	My_IncrementTime();
	CurrentTime = My_GetCurrentTime();
	My_CheckTaskWakeup(CurrentTime);
  My_SystemTickHandle();
	if (--My_Schedule_Time_Slice <= 0)
	{
		My_Schedule_Time_Slice = MY_SCHEDULE_TIME_SLICE_INIT_VALUE;
		My_Schedule();
		Ret =  pdTRUE;
	}

#ifdef MY_TIMER_USE
	My_TimerCheck(CurrentTime);
#endif

MY_SCHEDULER_INCREMENT_TICK_EXIT:
	MY_EXIT_CRITICAL();
	return Ret;
}

