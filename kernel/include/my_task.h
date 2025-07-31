#ifndef MY_TASK_H
#define MY_TASK_H
#include "my_list.h"
#include "my_types.h"
#include "my_config.h"

typedef void(*My_TaskFunction)(void *PrivateData);


#define MY_MIN_TASK_PRIORITY                    MY_MIN_PRIORITY
#define MY_TASK_MAGIC_NUMBER                    0xA5
#define MY_TASK_STACK_BOUNDARY                  0xA5A5A5A5

typedef struct __My_TaskControlBlock
{
	void * Stack;
	myList   StateList;
	my_uint8 Priority;
	my_uint8 BasePriority;
	my_uint8 *TaskName[MY_CONFIG_TASK_NAME_LEN];
	my_uint8 State;
	myList   EventSleepList;
	my_uint8 EventTimeoutWakeup;
	my_uint32 WakeUpTime;
}My_TCB;
	
typedef struct __My_TaskInitParameter
{
	My_TaskFunction  TaskEntry;
	my_uint8      Priority;
	my_uint8      Name[MY_CONFIG_TASK_NAME_LEN];
	my_uint32     StackSize;
	void          *PrivateData;
}My_TCBInitParameter;

typedef enum __My_Task_STATE {
	MY_TASK_READY = 0,
	MY_TASK_DELAY,
	MY_TASK_SUSPEND,
	MY_TASK_ENDLESS_BLOCKED,
	MY_TASK_TIMEOUT_BLOCKED,
	MY_TASK_UNKNOWN
} MY_TASK_STATE;
#define MY_TSK_HANDLE_TO_TCB(Handle)            ((My_TCB *)Handle)

my_uint32 My_TaskCreate(My_TCBInitParameter * Parameter, my_uint32 * TaskHandle);
my_uint32 My_TaskDelay(my_uint32 t);
my_uint32 My_TaskSuspend(my_uint32 TaskHandle);
my_uint32 My_TaskResume(my_uint32 TaskHandle);
my_uint32 My_TaskPrioritySet(my_uint32 TaskHandle, my_uint8 Prioriy);
my_uint8 My_TaskPriorityGet(my_uint32 TaskHandle);

void My_IdleTaskCreate(void);
my_uint8 My_TaskIncrementTick(my_uint32 CurrentTime);
My_TCB * My_GetHighestPriorityTask(void);


void My_TaskChangePriorityTemp(My_TCB * Tcb, my_uint8 NewPriority);
void My_TaskResumePriority(My_TCB* Tcb);
void My_TaskChangePriority(My_TCB * Tcb, my_uint8 NewPriority);

#endif // !MY_TASK_H


