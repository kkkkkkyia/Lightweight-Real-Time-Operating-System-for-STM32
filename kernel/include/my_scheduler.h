#ifndef MY_SCHEDULE_H
#define MY_SCHEDULE_H

#include "my_config.h"
#include "my_types.h"
#include "my_list.h"
#include "my_task.h"
static my_uint32 My_Schedule_Time_Slice;
typedef struct _My_TaskScheduler
{
	myList ReadyListHead[MY_PRIORITY_NUM];
	myList DelayListHead;
	myList SuspendListHead;
	myList BlockTimeoutListHead;

	my_uint32 PriorityActive;
	my_uint32 SchedulerSuspendNesting;
	my_uint32 ReSchedulePending;
}My_TaskScheduler;


typedef enum _MY_ScheduleReschPending
{
	NO_RESCH_PENDING = 0,
	RESCH_PENDING
}MY_SchedulePending;

typedef enum _My_SchedulerStateList {
	MY_READY_LIST = 0,
	MY_DELAY_LIST,
	MY_SUSPEND_LIST,
	MY_BLOCKED_TIMEOUT_LIST
} My_SchedulerStateList;

typedef enum _MY_BLOCK_TYPE
{
	MY_BLOCK_TYPE_ENDLESS = 0,
	MY_BLOCK_TYPE_TIMEOUT
}My_BlockType;

typedef enum _MY_BLOCK_SORT_TYPE
{
	MY_BLOCK_SORT_FIFO = 0,
	MY_BLOCK_SORT_PRIO
}My_BlockSortType;

typedef enum _MY_EventTimeoutWakeup
{
	MY_EVENT_NO_TIMEOUT = 0,
	MY_EVENT_WAIT_TIMEOUT
}My_EventTimeoutWakeup;
void My_AddTaskToReadyList(My_TCB * Tcb);
void My_AddTaskToEndlessBlockList(My_TCB * Tcb, myList *SleepHead, My_BlockSortType SortType);
void My_AddTaskToTimeSortList(My_TCB * Tcb, My_SchedulerStateList TargetList);
void My_AddTaskToDelayList(My_TCB * Tcb);
void My_AddTaskToTimeoutBlockedList(My_TCB * Tcb);
void My_AddTaskToSuspendList(My_TCB * Tcb);
void My_RemoveTaskFromReadyList(My_TCB * Tcb);

void My_TaskReadyToDelay(My_TCB * Tcb);
void My_TaskDelayToReady(My_TCB * Tcb);
void My_TaskReadyToSuspend(My_TCB * Tcb);
void My_TaskSuspendToReady(My_TCB * Tcb);
void My_TaskReadyToBlock(My_TCB * Tcb, myList* SleepHead, My_BlockType BlockType, My_BlockSortType SortType);
void My_TaskBlockToReady(My_TCB * Tcb);


void My_ClearPriorityActive(my_uint8 Priority);
void My_SetPriorityActive(my_uint8 Priority);
void My_ScheduleInit(void);
void My_Schedule(void);
void My_SchedulerSuspend(void);
void My_StartScheduler(void);
my_uint8 My_IsSchedulerSuspending(void);
void My_ScheduleResume(void);

void My_SystemTickHandle(void);
int My_Scheduler_IncrementTick(void);

#endif
