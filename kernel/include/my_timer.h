#ifndef MY_TIMER_H
#define MY_TIMER_H
#include "my_list.h"
#include "my_task.h"
#include "my_types.h"

typedef void(*My_TimerHandler)(void* Param);
typedef struct __My_TimerManager
{
	myList TimersList;
	my_uint32 NextWakeupTime;
}My_TimerManager;

typedef struct _My_Timer_Node
{
	myList List;
	my_uint8 Status;
	my_uint8 Mode;
	my_uint32 Interval;
	My_TimerHandler Handler;
	my_uint32 WakeupTime;
	my_uint8 Used;
	void *Param;
}My_Timer;

typedef enum _My_Timer_Mode
{
	MY_TIMER_ONESHOT = 0,
	MY_TIMER_AUTO_RELOAD 
}My_TimerMode;

typedef enum _My_TimerUsed
{
	MY_TIMER_UNUSED = 0,
	MY_TIMER_USED
}My_TImerUsed;

typedef enum __My_TimerStatus
{
	MY_TIMER_STOP = 0,
	MY_TIMER_RUNNING
}My_TimerSatus;
void My_TimerInit(void);
my_uint32 My_TimerCreate(my_uint32* TimerHandle, My_TimerMode Mode,my_uint32 Interval,My_TimerHandler TimerHandler,void* FuncParamer);
my_uint32 My_TimerStart(my_uint32 TimerHandle);
my_uint32 My_TimerStop(my_uint32 TimerHandle);
my_uint32 My_TimerDelete(my_uint32 TimerHandle);
void My_TimerCheck(my_uint32 CurrentTime);
void My_TimerTaskCreate(void);
#endif
