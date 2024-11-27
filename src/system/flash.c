/*******************************************************************************
* File Name: flash.c
*
* Description: Flash access functions and APIs for the PMG1 MCU.
*
* Related Document: See README.md
*
********************************************************************************
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

#include <string.h>
#include "cy_pdutils.h"
#include "cy_flash.h"
#include "flash.h"
#include "boot.h"

/*******************************************************************************
* Macro definitions
*******************************************************************************/
/*  Macro #0: rows 0x00-0x1ff, Macro #1: rows 0x200-0x3ff, macro # 2: rows 0x400-0x5ff  */
#define FLASH_GET_MACRO_FROM_ROW(row)        ((row) / CY_FLASH_SIZEOF_MACRO)

/* Keys used in SROM APIs. */
#define FLASH_PARAM_KEY_ONE                     (0xB6u)
#define FLASH_PARAM_KEY_TWO(x)                  (uint32_t)(0xD3u + x)

/* SROM API parameter opcodes */
#define FLASH_API_OPCODE_LOAD                   (0x04u)
#define FLASH_API_OPCODE_PROGRAM                (0x05u)
#define FLASH_API_OPCODE_SFLASH_WRITE           (0x18u)

/* SROM API parameter offset */
#define FLASH_PARAM_KEY_TWO_OFFSET              (0x08u)
#define FLASH_PARAM_ROW_NUM_OFFSET              (0x10u)
#define FLASH_PARAM_MACRO_OFFSET                (0x18u)

/* CPUSS SYSARG request start */
#define FLASH_CPUSS_SYSREQ_START                ((uint32_t) ((uint32_t) 0x1U << 31U))

/* CPUSS SYSARG return value mask. */
#define FLASH_CPUSS_SYSARG_RETURN_VALUE_MASK    (0xF0000000u)

/* CPUSS SYSARG success return value. */
#define FLASH_CPUSS_SYSARG_PASS_RETURN_VALUE    (0xA0000000u)

/* CPUSS parameter size */
#define FLASH_CPUSS_PARAM_SIZE                  (8u)


/*******************************************************************************
* Global variables
*******************************************************************************/
/* Whether flashing mode is enabled.*/
static bool glFlashModeEn = false;

/* Lowest flash row number that can be accessed.*/
static uint16_t glFlashAccessFirst = PMG1_FIRST_FLASH_ROW_NUM;

/* Highest flash row number that can be accessed.*/
static uint16_t glFlashAccessLast = PMG1_LAST_FLASH_ROW_NUM;

/* Flash row containing metadata for the alternate firmware image.*/
static uint16_t glFlashMetadataRow = PMG1_LAST_FLASH_ROW_NUM + 1;

/* Last boot loader flash row. Used for read protection.*/
static uint16_t glFlashBlLastRow = PMG1_LAST_FLASH_ROW_NUM;


/*******************************************************************************
* Function definitions
*******************************************************************************/
/*
 * This function invokes the SROM API to do a flash row write.
 * This function is used instead of the CySysFlashWriteRow, so as to avoid
 * the clock trim updates that are done as part of that API.
 */
static pmg1_status_t flash_trig_row_write(uint32_t row_num, uint8_t *data_p, bool is_sflash)
{
    volatile uint32_t params[(CY_FLASH_SIZEOF_ROW+ FLASH_CPUSS_PARAM_SIZE) / sizeof(uint32_t)];
    pmg1_status_t status = PMG1_STAT_SUCCESS;

    uint8_t intmask = SYS_CALL_MAP(Cy_SysLib_EnterCriticalSection)();
    uint32_t imosel = SRSSLT_CLK_IMO_SELECT;
    uint32_t clksel = SRSSLT_CLK_SELECT;


    /* If the IMO/HFCLK frequency is not 48 MHz, we have to change the frequency. */
    if ((imosel & SRSSLT_CLK_IMO_SELECT_FREQ_Msk) != 0x06)
    {
        SRSSLT_CLK_IMO_SELECT = 0x06;
        __NOP();
    }

    if ((clksel & (SRSSLT_CLK_SELECT_HFCLK_SEL_Msk | SRSSLT_CLK_SELECT_HFCLK_DIV_Msk)) != 0x00)
    {
        SRSSLT_CLK_SELECT &= ~(SRSSLT_CLK_SELECT_HFCLK_SEL_Msk | SRSSLT_CLK_SELECT_HFCLK_DIV_Msk);
        __NOP();
    }

    /* Connect the charge pump to IMO clock for flash write. */
#ifdef PAG1S
    SRSS_CLK_SELECT = (SRSS_CLK_SELECT & ~SRSS_CLK_SELECT_PUMP_SEL_Msk) | (1 << SRSS_CLK_SELECT_PUMP_SEL_Pos);
#else /* !PAG1S */
    SRSSLT_CLK_SELECT = (SRSSLT_CLK_SELECT & ~SRSSLT_CLK_SELECT_PUMP_SEL_Msk) | (1u << SRSSLT_CLK_SELECT_PUMP_SEL_Pos);
#endif /* PAG1S */

    /* Copy the data into the parameter buffer. */
    /* QAC suppression 0312: volatile qualifier for params[] is not mandatory as
     * Cy_PdUtils_MemCopy never reads params[], and it makes sure that each byte is written. */
    TIMER_CALL_MAP(Cy_PdUtils_MemCopy) ((uint8_t *)(&params[2]), (const uint8_t *)data_p, CY_FLASH_SIZEOF_ROW); /* PRQA S 0312 */

    /* Set the parameters for load data into latch operation. */
    params[0] = FLASH_PARAM_KEY_ONE |
        (FLASH_PARAM_KEY_TWO((FLASH_API_OPCODE_LOAD)) << FLASH_PARAM_KEY_TWO_OFFSET);
    params[1] = CY_FLASH_SIZEOF_ROW - 1u;
    /* If more than one flash macro is used, get the macro number using flash row number */
#if (CPUSS_SPCIF_FLASH_MACROS > 1)
    if (FLASH_GET_MACRO_FROM_ROW(row_num) != 0)
    {
        params[0] |= (1 << FLASH_PARAM_MACRO_OFFSET);
    }
#endif /* (CPUSS_SPCIF_FLASH_MACROS > 1) */

    CPUSS_SYSARG = (uint32_t)(&params[0]);
    CPUSS_SYSREQ = (FLASH_CPUSS_SYSREQ_START | FLASH_API_OPCODE_LOAD);

    __NOP();
    __NOP();
    __NOP();

    /* If load latch is successful. */
    if ((CPUSS_SYSARG & FLASH_CPUSS_SYSARG_RETURN_VALUE_MASK) == FLASH_CPUSS_SYSARG_PASS_RETURN_VALUE)
    {
        if (is_sflash)
        {
            /* Perform the sflash write. */
            params[0] = (FLASH_PARAM_KEY_ONE |
                    (FLASH_PARAM_KEY_TWO((FLASH_API_OPCODE_SFLASH_WRITE)) << FLASH_PARAM_KEY_TWO_OFFSET));
            params[1] = row_num;
            CPUSS_SYSARG = (uint32_t)(&params[0]);
            CPUSS_SYSREQ = (FLASH_CPUSS_SYSREQ_START | FLASH_API_OPCODE_SFLASH_WRITE);
        }
        else
        {
            /* Perform the flash write. */
            params[0] = ((row_num << FLASH_PARAM_ROW_NUM_OFFSET) | FLASH_PARAM_KEY_ONE |
                    (FLASH_PARAM_KEY_TWO((FLASH_API_OPCODE_PROGRAM)) << FLASH_PARAM_KEY_TWO_OFFSET));
            CPUSS_SYSARG = (uint32_t)(&params[0]);
            CPUSS_SYSREQ = (FLASH_CPUSS_SYSREQ_START | FLASH_API_OPCODE_PROGRAM);
        }

        __NOP();
        __NOP();
        __NOP();

        if ((CPUSS_SYSARG & FLASH_CPUSS_SYSARG_RETURN_VALUE_MASK) != FLASH_CPUSS_SYSARG_PASS_RETURN_VALUE)
        {
            status = PMG1_STAT_FAILURE;
        }
    }
    else
    {
        status = PMG1_STAT_FAILURE;
    }

    /* Disconnect the clock to the charge pump after flash write is complete. */
#ifdef PAG1S
    SRSSULT->clk_select = (SRSSULT->clk_select & ~CLK_SELECT_PUMP_SEL_MASK);
#else /* !PAG1S */
    SRSSLT_CLK_SELECT = (SRSSLT_CLK_SELECT & ~SRSSLT_CLK_SELECT_PUMP_SEL_Msk);
#endif /* PAG1S */


    /* Restore clock to earlier values. */
    SRSSLT_CLK_SELECT     = clksel;
    __NOP();
    SRSSLT_CLK_IMO_SELECT = imosel;
    __NOP();

    SYS_CALL_MAP(Cy_SysLib_ExitCriticalSection)(intmask);

    return status;
}

/**
 * @brief Write data in buffer to flash row at rowNum.
 * @buffer Buffer containing the data to be written to the flash row.
 * @rowNum Row number to be updated.
 */
pmg1_status_t flash_row_write ( uint8_t *buffer, uint16_t rowNum)
{
    uint32_t seqNum;
    uint16_t offset;

    /* Return device/stack not ready if flashing mode is disabled.*/
    if (!glFlashModeEn)
    {
        return PMG1_STAT_NOT_READY;
    }

    /* Return Bad input parameter if buffer is zero or row number is
    outside the accessible row.*/
    if ((buffer == 0) || (rowNum < glFlashAccessFirst) ||
        ((rowNum > glFlashAccessLast) && (rowNum != glFlashMetadataRow)))
    {
        return PMG1_STAT_BAD_PARAM;
    }

    /* Byte offset to the sequence number field in metadata.*/
    offset  = (PMG1_FLASH_ROW_SIZE - PMG1_FW_METADATA_SIZE + PMG1_FW_METADATA_BOOTSEQ_OFFSET);

#if PMG1_BOOTLOAD_ENABLE
    if (rowNum == PMG1_FW1_METADATA_ROW)
    {
        /* Set sequence number to 1 + that of FW2.*/
        seqNum = boot_get_boot_seq (PMG1_FW_MODE_FWIMAGE_2) + 1;
        ((uint32_t *)buffer)[offset / 4] = seqNum;
    }
    if (rowNum == PMG1_FW2_METADATA_ROW)
    {
        /* Set sequence number to 1 + that of FW1.*/
        seqNum = boot_get_boot_seq (PMG1_FW_MODE_FWIMAGE_1) + 1;
        ((uint32_t *)buffer)[offset / 4] = seqNum;
    }
#else
    /* Update the image boot sequence number value.*/
    if (rowNum == glFlashMetadataRow)
    {
        /* Set the sequence number for the newly updated firmware image to 1 + seq no. of the other image.*/
        if (glActiveFw == PMG1_FW_MODE_FWIMAGE_1)
        {
            seqNum = boot_get_boot_seq (PMG1_FW_MODE_FWIMAGE_1) + 1;
        }
        else
        {
            seqNum = boot_get_boot_seq (PMG1_FW_MODE_FWIMAGE_2) + 1;
        }
        ((uint32_t *)buffer)[offset / 4] = seqNum;
    }
#endif

     return flash_trig_row_write(rowNum, buffer, false);
}

/**
 * @brief Read data from flash row at row_num into buffer.
 * @buffer Buffer to read the flash data into.
 * @rowNum Flash row number to be read.
 */
int8_t flash_row_read (uint16_t rowNum, uint8_t *buffer)
{
     /* Return device/stack not ready if flashing mode is disabled.*/
    if (!glFlashModeEn)
    {
        return (int8_t)PMG1_STAT_NOT_READY;
    }

    /* We allow any row outside of the boot-loader to be read.*/
    if ((buffer == 0) || (rowNum <= glFlashBlLastRow) || (rowNum > PMG1_LAST_FLASH_ROW_NUM))
    {
        return (int8_t)PMG1_STAT_BAD_PARAM;
    }

    memcpy (buffer, (void *)(rowNum << PMG1_FLASH_ROW_SHIFT_NUM), PMG1_FLASH_ROW_SIZE);

    return (int8_t)PMG1_STAT_SUCCESS;
}

/**
 * @brief Clear the flash row at rowNum
 * @rowNum Flash row number to be cleared
 */
pmg1_status_t flash_row_clear (uint16_t rowNum)
{
    uint8_t buffer[PMG1_FLASH_ROW_SIZE] = {0};
    return flash_row_write (buffer, rowNum);
}

/**
 * @brief Check whether flashing mode has been entered.
 */
bool flash_access_get_status (uint8_t modes)
{
    return ((bool)((glFlashModeEn & modes) != 0u));
}

/**
 * @brief Enter flashing mode.
 * @enable Whether to enter/exit flashing mode.
 */
void flash_enter_mode (bool enable)
{
    glFlashModeEn = enable;
}

/**
 * @brief Check whether flashing mode has been entered.
 * @return Returns true if flashing mode has been entered, false otherwise.
 */
bool flash_access_enabled (void)
{
    return glFlashModeEn;
}

/**
 * @brief Set limits to the flash rows that can be accessed.
 * @startRow The lowest row number that can be accessed.
 * @lastRow The highest row number that can be accessed.
 * @mdRow Row containing metadata for the alternate firmware.
 * @blLastRow Last bootloader row. Rows above this can be read.
 */
void flash_set_access_limits ( uint16_t startRow, uint16_t lastRow,
                               uint16_t mdRow, uint16_t blLastRow)
{
    /* Store the access limits if they are valid.*/
    if ((startRow <= PMG1_LAST_FLASH_ROW_NUM) && (lastRow <= PMG1_LAST_FLASH_ROW_NUM))
    {
        glFlashAccessFirst = startRow;
        glFlashAccessLast  = lastRow;
        glFlashMetadataRow = mdRow;
    }

    /* Store the BL last row information.*/
    glFlashBlLastRow = blLastRow;
}

/* END OF FILE */
