#include "my_kernel.h"
#include "my_scheduler.h"
#include "my_time.h"
#include "my_mem.h"
#include "my_mutex.h"
#include "my_sem.h"
#include "my_queue.h"
#include "my_timer.h"
#include "my_config.h"
#include "my_printk.h"

void My_KernelInit(void)
{
	My_ScheduleInit();
	My_TimeInit();
	My_Mem_Init();
#if MY_SEM_USE
	My_SemInit();
#endif

#if MY_TIMER_USE
	My_TimerInit();
#endif
#if MY_QUEUE_USE
	My_Queue_Init();
#endif
	MY_PRINTK_INFO("My Kernel Init Finished!\n");

}
