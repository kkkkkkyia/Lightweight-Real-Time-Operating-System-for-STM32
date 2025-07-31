#ifndef MY_TIME_H
#define MY_TIME_H

#include "my_types.h"
#define MY_TIME_MAX MY_UINT32_MAX
#define MY_TSK_DLY_MAX                  (MY_TIME_MAX / 2)

#define MY_TIME_AFTER(a,b) (((long)a-(long)b) >0)
#define MY_TIME_BEFORE(a,b) (((long)a-(long)b) <0)
#define MY_TIME_AFTER_EQ(a,b) (((long)a-(long)b) >=0)
#define MY_TIME_BEFORE_EQ(a,b) (((long)a-(long)b) <=0)

void My_TimeInit(void);
void My_IncrementTime(void);
my_uint32 My_GetCurrentTime(void);
#endif

