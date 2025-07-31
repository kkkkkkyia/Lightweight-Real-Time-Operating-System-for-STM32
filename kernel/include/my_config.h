#ifndef MY_CONFIG_H
#define MY_CONFIG_H
	
#define MY_BYTE_ALIGNMENT 8
#define MY_BYTE_ALIGNMENT_MASK (MY_BYTE_ALIGNMENT-1)

/*****   PRIORITY CONFIG *****/
#define MY_PRIORITY_NUM 32
#define MY_MIN_PRIORITY (MY_PRIORITY_NUM-1)
#define MY_HIGHEST_PRIORITY 0

/*****TASK CONFIG*****/
#define MY_CONFIG_TASK_NAME_LEN 10

/****QUEUE CONFIG *****/
#ifndef MY_QUEUE_USE
	#define MY_QUEUE_USE 1
#else
	#define MY_QUEUE_USE 0
#endif
#define MY_QUEUE_MAX_NUM 10

/***** MUTEX CONFIG *****/
#ifndef MY_MUTEX_USE
	#define MY_MUTEX_USE 1
#endif
#define MY_MUTEX_MAX_NUM 10

/******* SEM CONFIG *******/
#ifndef MY_SEM_USE
	#define MY_SEM_USE 1
#endif
#define MY_SEM_MAX_NUM 10
#define MY_SEM_COUNT_MAX 0xfffe


/***  Timer ****/
#ifndef MY_TIMER_USE
	#define MY_TIMER_USE 1
#endif
#define MY_TIMER_TASK_PRIORITY 0
#define MY_TIMER_MAX_NUM 10
#define MY_TIMER_TASK_STACK_SIZE 1024

/***        IDLE TASK        ****/
#ifndef MY_IDLE_TASK_USE
	#define MY_IDLE_TASK_USE 1
#endif
#define MY_IDLE_TASK_PRIO               MY_MIN_PRIORITY
#define MY_IDLE_TASK_STACK_SIZE       (512 * MY_SIZE_BYTE)

/** PRINTK  LEVEL **/

#define MY_PRINTK_ERROR_LEVEL                       3
#define MY_PRINTK_WARNING_LEVEL                     2
#define MY_PRINTK_INFO_LEVEL                        1
#define MY_PRINTK_DEBUG_LEVEL                       0

#define MY_PRINTK_LEVEL                             (MY_PRINTK_DEBUG_LEVEL)
#define MY_USE_PRINTK                               1

/***                  FreeRTOSconfig                ***/
#define configUSE_PREEMPTION		1
#define configUSE_IDLE_HOOK			0
#define configUSE_TICK_HOOK			0
#define configCPU_CLOCK_HZ			( ( unsigned long ) 72000000 )	
#define configTICK_RATE_HZ			( ( TickType_t ) 1000 )
#define configMAX_TASK_NAME_LEN		( 16 )
#define configUSE_16_BIT_TICKS		0
#define configIDLE_SHOULD_YIELD		1

#define configUSE_CO_ROUTINES 		0
#define configMAX_CO_ROUTINE_PRIORITIES ( 2 )

#define configKERNEL_INTERRUPT_PRIORITY 		255
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	191
#define configLIBRARY_KERNEL_INTERRUPT_PRIORITY	15

#define pdFALSE                                  ( ( BaseType_t ) 0 )
#define pdTRUE                                   ( ( BaseType_t ) 1 )
/***                  FreeRTOSconfig                ***/



/**  Mem  **/

#define MEM_USE_STATIC

#ifdef MEM_USE_STATIC
	#define MEM_STATIC_SPACE static
#else 
	#define MEM_STATIC_SPACE
#endif
#define MY_CONFIG_TOTAL_MEM_SIZE (10*MY_SIZE_KB)


/**  time **/
#define MY_SCHEDULE_TIME_SLICE_INIT_VALUE 1 // 时间片长度
#define MY_CONFIG_TICK_COUNT_INIT_VALUE 0

#endif
