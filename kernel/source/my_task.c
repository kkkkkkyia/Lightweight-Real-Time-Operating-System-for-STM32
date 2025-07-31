#include "my_critical.h"
#include "my_error_code.h"
#include "my_mem.h"
#include "my_lib.h"
#include "my_arch.h"
#include "port.h"
#include "my_task.h"
#include "my_scheduler.h"
#include "my_time.h"
#include "my_printk.h"
#include "my_list.h"
extern My_TaskScheduler Scheduler;
extern my_uint8 TaskYield ;

My_TCB * volatile CurrentTCB = MY_NULL;
My_TCB * volatile SwitchNextTCB = MY_NULL;
static my_uint32  My_IdleTaskHandle = 0;


my_uint32 My_TaskCreate(My_TCBInitParameter * Parameter, my_uint32 * TaskHandle)
{
	my_uint32 Ret = MY_SUCCESS;
	My_TCB * Tcb = MY_NULL;
	if (Parameter->Priority > MY_MIN_PRIORITY)
	{
		return MY_TASK_PRIO_OUT_OF_RANGE;
	}
	MY_ENTER_CRITICAL();
	Tcb = (My_TCB *)My_Malloc(sizeof(My_TCB));
	if (Tcb == MY_NULL)
	{
		Ret  = MY_NOT_ENOUGH_MEM_FOR_TASK_CREATE;
		goto NOT_ENOUGH_MEM;
	}
	Tcb->Stack = My_Malloc(Parameter->StackSize);
	if (Tcb->Stack == MY_NULL)
	{
		My_Free(Tcb);
		Ret = MY_NOT_ENOUGH_MEM_FOR_TASK_CREATE;
		goto NOT_ENOUGH_MEM;
	}
	My_MemSet(Tcb->Stack, MY_TASK_MAGIC_NUMBER, Parameter->StackSize);
	Tcb->BasePriority = Parameter->Priority;
	Tcb->Priority = Parameter->Priority;
	My_MemCopy(Tcb->TaskName, Parameter->Name, MY_CONFIG_TASK_NAME_LEN);
	Tcb->TaskName[MY_CONFIG_TASK_NAME_LEN - 1] = 0x00;

	Tcb->Stack = ARCH_PrepareStack(Tcb->Stack, Parameter);
	Tcb->State = MY_TASK_UNKNOWN;
	My_AddTaskToReadyList(Tcb);
	Tcb->WakeUpTime = MY_TIME_MAX;

	*TaskHandle = (my_uint32)Tcb;
	MY_PRINTK_INFO("Create Task Name:[%s], Priority:[%d]\n", (const char *)Tcb->TaskName, Tcb->Priority);
	MY_EXIT_CRITICAL();
	return MY_SUCCESS;

NOT_ENOUGH_MEM:
	MY_EXIT_CRITICAL();
	return Ret;
}

my_uint32 My_TaskDelay(my_uint32 t)
{
	my_uint32 Ret = MY_SUCCESS;
	MY_ENTER_CRITICAL();

	if (ATCH_IsInterruptContext())
	{
		Ret = MY_TSK_DLY_IN_INTR_CONTEXT;
		goto My_Task_Delay_Exit;
	}
	if (My_IsSchedulerSuspending())
	{
		Ret = MY_TSK_DLY_IN_SCH_SUSPEND;
		goto My_Task_Delay_Exit;
	}

	if ((t == 0) || (t >= MY_TSK_DLY_MAX))
	{
		Ret = MY_TSK_DLY_TICK_INVALID;
		goto My_Task_Delay_Exit;
	}
	CurrentTCB->WakeUpTime = My_GetCurrentTime() + t;
	My_TaskReadyToDelay(CurrentTCB);
	My_Schedule();

My_Task_Delay_Exit:
	MY_EXIT_CRITICAL();
	return Ret;
}


my_uint32 My_TaskSuspend(my_uint32  TaskHandle)
{
	My_TCB * Tcb = MY_TSK_HANDLE_TO_TCB(TaskHandle);
	my_uint32 Ret = MY_SUCCESS;

	MY_ENTER_CRITICAL();
	if (Tcb == MY_NULL)
	{
		Ret = MY_NULL_POINTER;
		goto MY_TASK_SUSPEND_EXIT;
	}
	if (Tcb->State == MY_TASK_SUSPEND)
	{
		Ret = MY_SUSPEND_CUR_TSK_IN_SCH_SUSPEND;
		goto MY_TASK_SUSPEND_EXIT;
	}

	if (Tcb == CurrentTCB)
	{
		if (ATCH_IsInterruptContext())
		{
			Ret = MY_TSK_DLY_IN_INTR_CONTEXT;
			goto MY_TASK_SUSPEND_EXIT;
		}
		if (My_IsSchedulerSuspending())
		{
			Ret = MY_TSK_DLY_IN_SCH_SUSPEND;
			goto MY_TASK_SUSPEND_EXIT;
		}
	}

	if (Tcb == CurrentTCB)
	{
		My_Schedule();
	}

MY_TASK_SUSPEND_EXIT:
	MY_EXIT_CRITICAL();
	return Ret;
}
my_uint32 My_TaskResume(my_uint32  TaskHandle)
{
	My_TCB * Tcb = MY_TSK_HANDLE_TO_TCB(TaskHandle);
	my_uint32 Ret = MY_SUCCESS;
	MY_ENTER_CRITICAL();

	if (Tcb == MY_NULL)
	{
		Ret = MY_NULL_POINTER;
		goto MY_TASK_RESUME_EXIT;
	}
	if (Tcb == CurrentTCB)
	{
		Ret = MY_RESUME_CUR_TSK;
		goto MY_TASK_RESUME_EXIT;
	}

	if (Tcb->State != MY_TASK_SUSPEND)
	{
		Ret = MY_RESUME_TSK_NOT_IN_SUSPEND;
		goto MY_TASK_RESUME_EXIT;
	}

	My_TaskSuspendToReady(Tcb);
	if (Tcb->Priority > CurrentTCB->Priority)
	{
		My_Schedule();
	}

MY_TASK_RESUME_EXIT:
	MY_EXIT_CRITICAL();
	return Ret;
}
my_uint32 My_TaskPrioritySet(my_uint32  TaskHandle, my_uint8 NewPriority)
{
	My_TCB * Tcb = MY_TSK_HANDLE_TO_TCB(TaskHandle);
	my_uint32 Ret = MY_SUCCESS;
	int NeedReSchedule = 0;
	if (Tcb == MY_NULL)
	{
		return MY_NULL_POINTER;
		
	}
	if (NewPriority == Tcb->Priority)
	{
		return MY_SET_SAME_PRIO;
	}
	if (NewPriority > MY_MIN_PRIORITY)
	{
		return MY_TASK_PRIO_OUT_OF_RANGE;
	}
	MY_ENTER_CRITICAL();

	if (Tcb == CurrentTCB)
	{
		if(NewPriority < Tcb->Priority)
			NeedReSchedule = 1;
	}
	else
	{
		if (NewPriority < Tcb->Priority && NewPriority < CurrentTCB->Priority && Tcb->State == MY_TASK_READY)
			NeedReSchedule = 1;
	}
	My_TaskChangePriority(Tcb, NewPriority);
	if (NeedReSchedule)
		My_Schedule();
	MY_EXIT_CRITICAL();
	return Ret;
}

void My_TaskChangePriorityTemp(My_TCB * Tcb, my_uint8 NewPriority)
{
	Tcb->Priority = NewPriority;
	if (Tcb->State == MY_TASK_READY)
	{
		My_RemoveTaskFromReadyList(Tcb);
		My_AddTaskToReadyList(Tcb);
	}
}

void My_TaskResumePriority(My_TCB* Tcb)
{
	Tcb->Priority = Tcb->BasePriority;	
	if (Tcb->State == MY_TASK_READY)
	{
		My_RemoveTaskFromReadyList(Tcb);
		My_AddTaskToReadyList(Tcb);
	}
}

my_uint8 My_TaskIncrementTick(my_uint32 CurrentTime)
{
	return 0;
}
void My_TaskChangePriority(My_TCB * Tcb, my_uint8 NewPriority)
{
	Tcb->Priority = NewPriority;
	Tcb->BasePriority = NewPriority;	
	if (Tcb->State == MY_TASK_READY)
	{
		My_RemoveTaskFromReadyList(Tcb);
		My_AddTaskToReadyList(Tcb);
	}
}

my_uint8 My_TaskPriorityGet(my_uint32  TaskHandle)
{
	My_TCB * Tcb = MY_TSK_HANDLE_TO_TCB(TaskHandle);
	my_uint8 Priority = 0;

	MY_ENTER_CRITICAL();
	Priority = Tcb->Priority;
	MY_EXIT_CRITICAL();
	return Priority;

}

void My_IdleTask(void *Parameter)
{
	while (1)
	{
		//printf(">> Idle Task Runing\n");
	}
}

void My_IdleTaskCreate(void)
{
	My_TCBInitParameter* Parameter = (My_TCBInitParameter*)My_Malloc(sizeof(My_TCBInitParameter));
	Parameter->Name[0] = 'I';
	Parameter->Name[1] = 'D';
	Parameter->Name[2] = 'L';
	Parameter->Name[3] = 'E';
	Parameter->Name[4] = 0x00;
	Parameter->Priority = MY_IDLE_TASK_PRIO;
	Parameter->PrivateData = MY_NULL;
	Parameter->StackSize = MY_IDLE_TASK_STACK_SIZE;
	Parameter->TaskEntry = My_IdleTask;
	My_TaskCreate(Parameter, &My_IdleTaskHandle);
	My_Free(Parameter);
	return;
}
My_TCB * My_GetHighestPriorityTask(void)
{
	my_uint8 TargetPriority = MY_HIGHEST_PRIORITY;
	My_TCB * TargetTcb = MY_NULL;
	for (; TargetPriority <= MY_MIN_PRIORITY; TargetPriority++)
	{
		if (Scheduler.PriorityActive & (0x01 << TargetPriority))
			break;
	}
	TargetTcb = ListFirstEntry(&Scheduler.ReadyListHead[TargetPriority], My_TCB, StateList);
	return TargetTcb;
}
