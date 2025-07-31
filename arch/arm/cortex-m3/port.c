/*
 * FreeRTOS Kernel V10.5.1
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/*-----------------------------------------------------------
* Implementation of functions defined in portable.h for the ARM CM3 port.
*----------------------------------------------------------*/

/* Scheduler includes. */
#include "port.h"

#include "my_scheduler.h"

/*
 * Setup the timer to generate the tick interrupts.  The implementation in this
 * file is weak to allow application writers to change the timer used to
 * generate the tick interrupt.
 */

/*
 * Start first task is a separate function so it can be tested in isolation.
 */
static void prvStartFirstTask( void );

/*
 * Used to catch tasks that attempt to return from their implementing function.
 */
static void prvTaskExitError( void );



static UBaseType_t uxCriticalNesting = 0xaaaaaaaa;


/*
 * See header file for description.
 */
void * pxPortInitialiseStack( my_uint32 * pxTopOfStack,
                                     my_uint32 pxCode,
                                     void * pvParameters )
{
    /* Simulate the stack frame as it would be created by a context switch
     * interrupt. */
    pxTopOfStack--;                                                      /* Offset added to account for the way the MCU uses the stack on entry/exit of interrupts. */
    *pxTopOfStack = portINITIAL_XPSR;                                    /* xPSR */
    pxTopOfStack--;
    *pxTopOfStack = ( ( my_uint32 ) pxCode ) & portSTART_ADDRESS_MASK; /* PC */
    pxTopOfStack--;
    *pxTopOfStack = ( my_uint32 ) prvTaskExitError;                    /* LR */

    pxTopOfStack -= 5;                                                   /* R12, R3, R2 and R1. */
    *pxTopOfStack = ( my_uint32 ) pvParameters;                        /* R0 */
    pxTopOfStack -= 8;                                                   /* R11, R10, R9, R8, R7, R6, R5 and R4. */

    return pxTopOfStack;
}
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

__asm void vPortSVCHandler( void )
{
/* *INDENT-OFF* */
    PRESERVE8
    ldr r3, =CurrentTCB   /* Restore the context. */
    ldr r1, [ r3 ] /* Use pxCurrentTCBConst to get the pxCurrentTCB address. */
    ldr r0, [ r1 ]           /* The first item in pxCurrentTCB is the task top of stack. */
    ldmia r0 !, { r4 - r11 } /* Pop the registers that are not automatically saved on exception entry and the critical nesting count. */
    msr psp, r0 /* Restore the task stack pointer. */
    isb
    mov r0, # 0
    msr basepri, r0
    orr r14, #0xd
    bx r14
/* *INDENT-ON* */
}
/*-----------------------------------------------------------*/

__asm void prvStartFirstTask( void )
{
/* *INDENT-OFF* */
    PRESERVE8

    /* Use the NVIC offset register to locate the stack. */
    ldr r0, =0xE000ED08
    ldr r0, [ r0 ]
    ldr r0, [ r0 ]

    /* Set the msp back to the start of the stack. */
    msr msp, r0
    /* Globally enable interrupts. */
    cpsie i
    cpsie f
    dsb
    isb
    /* Call SVC to start the first task. */
    svc 0
    nop
    nop
/* *INDENT-ON* */
}
/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
my_uint32 xPortStartScheduler( void )
{
    /* Make PendSV and SysTick the lowest priority interrupts. */
    portNVIC_SHPR3_REG |= portNVIC_PENDSV_PRI;

    portNVIC_SHPR3_REG |= portNVIC_SYSTICK_PRI;

    /* Start the timer that generates the tick ISR.  Interrupts are disabled
     * here already. */
    vPortSetupTimerInterrupt();
		uxCriticalNesting = 0;
    /* Start the first task. */
    prvStartFirstTask();

    /* Should not get here! */
    return 0;
}


void vPortEnterCritical( void )
{
    portDISABLE_INTERRUPTS();
	uxCriticalNesting++;

    /* This is not the interrupt safe version of the enter critical function so
     * assert() if it is being called from an interrupt context.  Only API
     * functions that end in "FromISR" can be used in an interrupt.  Only assert if
     * the critical nesting count is 1 to protect against recursive calls if the
     * assert function also uses a critical section. */
}
/*-----------------------------------------------------------*/

void vPortExitCritical( void )
{
	uxCriticalNesting--;
	if(uxCriticalNesting==0)
		portENABLE_INTERRUPTS();
}
/*-----------------------------------------------------------*/
static void prvTaskExitError( void )
{
    /* A function that implements a task must not exit or attempt to return to
     * its caller as there is nothing to return to.  If a task wants to exit it
     * should instead call vTaskDelete( NULL ).
     *
     * Artificially force an assert() to be triggered if configASSERT() is
     * defined, then stop here so application writers can catch the error. */
    portDISABLE_INTERRUPTS();

    for( ; ; )
    {
			
    }
}
__asm void xPortPendSVHandler( void )
{
    extern CurrentTCB;
    extern SwitchNextTCB;
/* *INDENT-OFF* */
    PRESERVE8
    mrs r0, psp
    isb
    ldr r3, =CurrentTCB /* Get the location of the current TCB. */
    ldr r2, [ r3 ]

    stmdb r0 !, { r4 - r11 } /* Save the remaining registers. */
    str r0, [ r2 ] /* Save the new top of stack into the first member of the TCB. */

		
    /* Get the next TCB stack pointer */
    ldr     r3, =SwitchNextTCB
		ldr     r1, [r3]
    ldr     r0, [r1]

    /* Pop the core registers. */
    ldmia r0 !, { r4 - r11 }
		msr     psp, r0
		isb
		LDR R0,=CurrentTCB
		STR r1,[R0]
    bx r14   //  R14 (LR register)
    nop
/* *INDENT-ON* */
}
/*-----------------------------------------------------------*/
void My_ContextSwitch(void)
{
	portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
}

void xPortSysTickHandler( void )
{
    /* The SysTick runs at the lowest interrupt priority, so when this interrupt
     * executes all interrupts must be unmasked.  There is therefore no need to
     * save and then restore the interrupt mask value as its value is already
     * known - therefore the slightly faster vPortRaiseBASEPRI() function is used
     * in place of portSET_INTERRUPT_MASK_FROM_ISR(). */
    vPortRaiseBASEPRI();
    {
        /* Increment the RTOS tick. */
        if( My_Scheduler_IncrementTick() ==pdTRUE )
        {
            /* A context switch is required.  Context switching is performed in
             * the PendSV interrupt.  Pend the PendSV interrupt. */
					portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
        }
    }

    vPortClearBASEPRIFromISR();
}



void vPortSetupTimerInterrupt( void )
{
		/* Calculate the constants required to configure the tick interrupt. */
		/* Stop and clear the SysTick. */
		portNVIC_SYSTICK_CTRL_REG = 0UL;
		portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

		/* Configure SysTick to interrupt at the requested rate. */
		portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
		portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT );
}


