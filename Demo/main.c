#include "my_mem.h"
#include <stdio.h>
#include "my_scheduler.h"
#include "my_task.h"
#include "my_lib.h"
#include "my_critical.h"
#include "my_queue.h"
#include "my_error_code.h"
#include "my_sem.h"
#include "my_mutex.h"
#include "my_kernel.h"
#include "platform.h"
#include "my_timer.h"

typedef struct __Data
{
	int index;
	int age;
	int sex;
	int goat;
}myData;

my_uint32 Task1Handle;
my_uint32 Task2Handle;
my_uint32 Task3Handle;

my_uint32 QueueHandle1;
my_uint32 MutexHandle1;
my_uint32 SemHandle1;
my_uint32  TimerHandle1;
void func1(void* param)
{
	int cnt =0;
	while(1)
	{
		printf("Task1 Runing\n");
		My_SemRelease(SemHandle1);
		if(cnt++%3==0)
		My_SemRelease(SemHandle1);
		My_TaskDelay(1);
	}
}

void func2(void* param)
{
	int cnt=10;
	while(1)
	{
		My_SemGet(SemHandle1);
		printf("Task2 is running\n");
		if(cnt--==0)
		{
			cnt=10;
			My_TimerStart(TimerHandle1);
		}
	}
}
void func3(void* param)
{
	while(1)
	{
		My_SemGet(SemHandle1);
		printf("Task 3running\n");
	}
}

void TimerHandleFunc(void*Param)
{
	char *str = (char *)Param;
	printf("Timer Runing Param:%s\n",str);
}

int main(void)
{
	my_uint32 Ret = 0;
	PlatformInit();
	printf("Start Running MyRTOS\n");
	My_KernelInit();
	Ret = My_QueueCreate(&QueueHandle1,sizeof(myData),10);
	printf("QueueCreate Ret:%d\n",Ret);
	
	Ret = My_MutexCreate(&MutexHandle1);
	printf("MutexCreate Ret:%d\n",Ret);
	Ret = My_SemCreate(&SemHandle1,0);
	printf("SemCreate Ret:%d\n",Ret);
	
	My_TimerCreate(&TimerHandle1,MY_TIMER_ONESHOT,10,TimerHandleFunc,"Timer Paramer");
	
	My_TCBInitParameter Task1Parameter;
	Task1Parameter.Priority = 3;
	Task1Parameter.StackSize = 1024;
	Task1Parameter.TaskEntry = func1;
	My_MemCopy(Task1Parameter.Name,"Task1",MY_CONFIG_TASK_NAME_LEN);
	Task1Parameter.PrivateData = "Task1PrivateData";
	My_TaskCreate(&Task1Parameter,&Task1Handle);
	
	
	My_TCBInitParameter Task2Parameter;
	Task2Parameter.Priority = 3;
	Task2Parameter.StackSize = 1024;
	Task2Parameter.TaskEntry = func2;
	My_MemCopy(Task2Parameter.Name,"Task2",MY_CONFIG_TASK_NAME_LEN);
	Task2Parameter.PrivateData = "Task2PrivateData";
	My_TaskCreate(&Task2Parameter,&Task2Handle);
	
	My_TCBInitParameter Task3Parameter;
	Task3Parameter.Priority = 3;
	Task3Parameter.StackSize = 1024;
	Task3Parameter.TaskEntry = func3;
	My_MemCopy(Task3Parameter.Name,"Task3",MY_CONFIG_TASK_NAME_LEN);
	Task3Parameter.PrivateData = "Task3PrivateData";
	My_TaskCreate(&Task3Parameter,&Task3Handle);
	
	
	My_StartScheduler();
	return 0;
}


/**
 *
 * Timer 2 's Interrupt Handle Function, used by start.s
 */
void vTimer2IntHandler( void )
{
	
}


