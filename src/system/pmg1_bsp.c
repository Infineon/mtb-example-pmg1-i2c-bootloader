/******************************************************************************
* File Name: pmg1_bsp.c
*
* Description: This is device BSP initialization source file for the PMG1
*              MCU I2C BOOTLOADER Example for ModusToolBox.
*
* Related Document: See README.md
*
*******************************************************************************
* Copyright 2023-2024, Cypress Semiconductor Corporation (an Infineon company) or
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


#include "cycfg.h"
#include "pmg1_bsp.h"
#include "cycfg_system.h"
#include "system_cat2.h"

#define CY_CFG_SYSCLK_CLKSYS_FREQ_MHZ    ((uint32_t)(CY_CLK_SYSTEM_FREQ_HZ / 1000000UL))
#define CY_CFG_SYSCLK_HFCLK_SOURCE       CY_SYSCLK_CLKHF_IN_IMO
#define CY_CFG_SYSCLK_HFCLK_DIVIDER      CY_SYSCLK_NO_DIV
#define CY_CFG_SYSCLK_CLKSYS_DIVIDER     CY_SYSCLK_NO_DIV

#define CY_CFG_SYSCLK_FREQ_SCALER        (1000000UL)
#define CY_CFG_IMO_LOC_FREQ              (((uint32_t)CY_CFG_SYSCLK_IMO_FREQ - \
                                         (uint32_t)CY_SYSCLK_IMO_24MHZ) / \
                                          CY_CFG_SYSCLK_FREQ_SCALER)

#define CY_DELAY_MS_OVERFLOW_THRESHOLD   (0x8000u)
#define CY_DELAY_1K_THRESHOLD            (1000u)
#define CY_DELAY_1M_THRESHOLD            (1000000u)

#define CY_DELAY_FREQ_KHZ                (CY_SYSLIB_DIV_ROUNDUP(CY_CLK_SYSTEM_FREQ_HZ,\
                                          CY_DELAY_1K_THRESHOLD))

#define CY_DELAY_FREQ_MHZ                ((uint8_t)CY_SYSLIB_DIV_ROUNDUP(CY_CLK_SYSTEM_FREQ_HZ, \
                                          CY_DELAY_1M_THRESHOLD))

#define CY_DELAY_FREQ_32KMS              (CY_DELAY_MS_OVERFLOW_THRESHOLD * \
                                          CY_SYSLIB_DIV_ROUNDUP(CY_CLK_SYSTEM_FREQ_HZ,\
                                          CY_DELAY_1K_THRESHOLD))

static void Set_ImoFrequency(void)
{
    /* Convert the frequency value in Hz into the SFLASH.IMO_TRIM register index */
    uint32_t locFreq = CY_CFG_IMO_LOC_FREQ;
    uint32_t intStat = Cy_SysLib_EnterCriticalSection();

    /* Set IMO to 24 MHz */
    SRSSLT_CLK_IMO_SELECT = 0UL;

    /* Apply coarse trim */
    SRSSLT_CLK_IMO_TRIM1 = SFLASH_IMO_TRIM_LT(locFreq);

    /* Zero out fine trim */
    SRSSLT_CLK_IMO_TRIM2 = 0UL;

    /* Apply TC trim */
    SRSSLT_CLK_IMO_TRIM3 = SFLASH_IMO_TCTRIM_LT(locFreq);

    /* Convert the SFLASH.IMO_TRIM register index into the frequency bitfield value */
    locFreq >>= 2UL;

    Cy_SysLib_DelayCycles(50UL);

    if (0UL != locFreq)
    {
        /* Select nearby intermediate frequency */
        CY_REG32_CLR_SET(SRSSLT_CLK_IMO_SELECT, SRSSLT_CLK_IMO_SELECT_FREQ, locFreq - 1UL);

        Cy_SysLib_DelayCycles(50UL);

        /* Make small step to final frequency */
        CY_REG32_CLR_SET(SRSSLT_CLK_IMO_SELECT, SRSSLT_CLK_IMO_SELECT_FREQ, locFreq);
    }

    Cy_SysLib_ExitCriticalSection(intStat);
}

static void pmg1_system_init(void)
{
    cy_en_sysclk_status_t status;
    /* Set worst case memory wait states (48 MHz), will update at the end */
    Cy_SysLib_SetWaitStates(48);

    /* Initialize IMO frequency */
    Cy_SysClk_ImoEnable();
    (void)Cy_SysClk_ClkPumpSetSource(CY_SYSCLK_PUMP_IN_GND);
    Set_ImoFrequency();

    /* Initialize HFCLK */
    status = Cy_SysClk_ClkHfSetSource(CY_CFG_SYSCLK_HFCLK_SOURCE);
    if (CY_SYSCLK_SUCCESS != status)
    {
        cycfg_ClockStartupError(CY_CFG_SYSCLK_HF_SRC_ERROR, status);
    }
    Cy_SysClk_ClkHfSetDivider(CY_CFG_SYSCLK_HFCLK_DIVIDER);

    /* Initialize SysClock */
    Cy_SysClk_ClkSysSetDivider(CY_CFG_SYSCLK_CLKSYS_DIVIDER);

    /* Set accurate flash wait states */
    Cy_SysLib_SetWaitStates(CY_CFG_SYSCLK_CLKSYS_FREQ_MHZ);

    /* Update System Core Clock values for correct Cy_SysLib_Delay functioning */
    SystemCoreClock = CY_CLK_SYSTEM_FREQ_HZ;
    cy_delayFreqKhz = CY_DELAY_FREQ_KHZ;
    cy_delayFreqMhz = CY_DELAY_FREQ_MHZ;
    cy_delay32kMs = CY_DELAY_FREQ_32KMS;
}

void pmg1_bsp_init(void)
{
    /* System clock is initialized in this function. Device Configurator change
     * done for system clock will not impact here since some of the auto generated
     * macros are part of source file. Due to this re-defined the macros here.
     */
    pmg1_system_init();
    init_cycfg_clocks();
    init_cycfg_peripherals();
    init_cycfg_pins();
}
