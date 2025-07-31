#include "my_time.h"
#include "my_config.h"

my_uint32 volatile My_CurrentTime = 0;

void My_TimeInit(void)
{
	My_CurrentTime = MY_CONFIG_TICK_COUNT_INIT_VALUE;
}

void My_IncrementTime(void)
{
	My_CurrentTime++;
}
my_uint32 My_GetCurrentTime(void)
{
	return My_CurrentTime;
}

