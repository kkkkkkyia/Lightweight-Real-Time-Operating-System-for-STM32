#ifndef MY_MUTEX_H
#define MY_MUTEX_H

#include "my_task.h"
typedef struct _MY_MUTEX
{
	myList SleepList;
	My_TCB * Owner; // 获得该锁的任务
	my_uint32 OwnerHoldCount; //是否被获取
	my_uint8 OwnerPriority; // 获取该锁的优先级
	my_uint8 Used;  // 该锁是否从池中取出被初始化为正在使用
}My_Mutex;


typedef enum __My_MutexUsed
{
	MY_MUTEX_UNUSED=0,
	MY_MUTEX_USED
}My_MutexUsed;


void My_MutexInit(void);
my_uint32 My_MutexCreate(my_uint32 *MutexHandle);
my_uint32 My_MutexLock(my_uint32 MutexHandle);
my_uint32 My_MutexLockTimeout(my_uint32 MutexHandle, my_uint32 Timeout);
my_uint32 My_MutexTryLock(my_uint32 MutexHandle);
my_uint32 My_MutexUnlock(my_uint32 MutexHandle);
my_uint32 My_MutexDestroy(my_uint32 MutexHandle);


#endif
