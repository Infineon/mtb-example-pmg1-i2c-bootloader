/******************************************************************************
* File Name: timer.c
*
* Description: This file defines timer functions and APIs for the PMG1 MCU.
*
* Related Document: See README.md
*
*******************************************************************************
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

#include <cy_pdl.h>
#include "timer.h"
#include "pmg1_bsp.h"

/*******************************************************************************
* Macro Definition
*******************************************************************************/
/* SysTick reload value for 1ms time period. */
#define SYSTICK_RELOAD_VALUE  ((CY_CLK_SYSTEM_FREQ_HZ / 1000u) - 1u)

/*******************************************************************************
* Global Variables
*******************************************************************************/
/* Variable to store the timeout value. */
static uint16_t glTimeoutPeriod;

/* Timer expired callback function pointer. */
static timer_cb_t glTimerCb;

/*******************************************************************************
* Function Definition
*******************************************************************************/
static void timer_interrupt_handler() {
    /* Decrement the timeout value of the active timer and check if the timeout
     * has occurred. If timeout has occurred, stop the timer and perform the
     * necessary actions. */
    if(glTimeoutPeriod > 0)
    {
        glTimeoutPeriod--;
    }
    else
    {
        glTimeoutPeriod = 0;

        if(NULL != glTimerCb)
        {
            glTimerCb();
        }

        /* Disable the SYSTICK interrupt. */
        Cy_SysTick_Disable() ;
    }
}

void timer_init(void)
{
    glTimeoutPeriod = 0;
    glTimerCb = NULL;

    /* Disable the timer and configure the SysTick interrupt interval to 1ms*/
    Cy_SysTick_Disable() ;
    Cy_SysTick_SetReload(SYSTICK_RELOAD_VALUE);
}

void timer_start(uint16_t timeout, timer_cb_t cb)
{
    uint32_t state;

    /* Enter critical section */
    state = Cy_SysLib_EnterCriticalSection();

    glTimeoutPeriod = timeout;
    glTimerCb = cb;

    /* Clear the counter */
    Cy_SysTick_Clear() ;

    /* Start the timer and enable the interrupt. */
    Cy_SysTick_SetClockSource(CY_SYSTICK_CLOCK_SOURCE_CLK_CPU);
    Cy_SysTick_Enable();

    /* Exit critical section. */
    Cy_SysLib_ExitCriticalSection(state);
}

void timer_stop(void)
{
    uint32_t state;

    /* Enter critical section */
    state = Cy_SysLib_EnterCriticalSection();

    glTimeoutPeriod = 0;
    glTimerCb = NULL;

    /* Stop the timer interrupt */
    Cy_SysTick_Disable() ;

    /* Exit critical section. */
    Cy_SysLib_ExitCriticalSection(state);

}


/* Timer ISR is called every 1ms. If there is any soft timer ON
   In the ISR all active timer instances are decremented and then checked
   for timeouts. In case of timeout the timer is stopped and de-allocated.
   In case, a timer needs to be re-started, it can be done from the callback.

   On timer expire events are also raised here for various modules
   */
void SysTick_Handler(void)
{
    /* Clear the count flag by reading the CM0->syst_csr register. */
    if(Cy_SysTick_GetCountFlag())
    {
        /* Invoke the timer handler. */
        timer_interrupt_handler ();
    }
}

/*End of File*/
