/******************************************************************************
* File Name: timer.h
*
* Description: This file is software timers header file for the PMG1 MCU.
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

#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
* Data types
*******************************************************************************/
/**
 * Timer callback function.
 *
 * This callback function is invoked on timer expire and should be treated as
 * interrupt.
 */
typedef void (*timer_cb_t)(void);

/*******************************************************************************
* Function definitions
*******************************************************************************/

/**
 * Initialize Software timer module. This function must be called once before
 * using other timer APIs
 */
void timer_init(void);

/**
 * Start a soft timer
 * @param timeout: timer period
 * @param cb: callback function pointer
 */
void timer_start(uint16_t timeout, timer_cb_t cb);

/**
 * Stop a soft timer
 */
void timer_stop(void);

#endif /* TIMER_H_ */

/* EOF */
