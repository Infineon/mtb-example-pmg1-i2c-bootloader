/******************************************************************************
* File Name: flash.h
*
* Description: This header file is flash access header for the PMG1 MCU.
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

#ifndef __FLASH_H__
#define __FLASH_H__

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "cy_flash.h"
#include "status.h"

/*******************************************************************************
* Macro definitions
*******************************************************************************/

/** PMG1 FLASH OPTIONS **/
/* Only the boot-loader last row and the configuration table size is expected
 * to change to match the project requirement. With a fixed boot-loader, none
 * of this fields should be modified.
 */

/**  Flash row size for DEVICE_MODE register
 * 0 � 128 bytes
 * 1 � 256 bytes
 * 3 � 64 bytes
 */
#if (CY_FLASH_SIZEOF_ROW == 64U)
/** Flash row shift number. */
#define PMG1_FLASH_ROW_SHIFT_NUM            (6u)
#define PMG1_FLASH_ROW_SIZE_DEV_MODE_VAL    (3u)
#elif (CY_FLASH_SIZEOF_ROW == 128U)
/** Flash row shift number. */
#define PMG1_FLASH_ROW_SHIFT_NUM            (7u)
#define PMG1_FLASH_ROW_SIZE_DEV_MODE_VAL    (0u)
/** Flash row shift number. */
#elif (CY_FLASH_SIZEOF_ROW == 256U)
/** Flash row shift number. */
#define PMG1_FLASH_ROW_SHIFT_NUM            (8u)
#define PMG1_FLASH_ROW_SIZE_DEV_MODE_VAL    (1u)
#endif /* CY_FLASH_SIZEOF_ROW */

/* Total size of device flash. */
#define PMG1_FLASH_SIZE                     (CY_FLASH_SIZE)

/* First row number of flash. */
#define PMG1_FIRST_FLASH_ROW_NUM            (CY_FLASH_BASE)

/* Last row number of flash. */
#define PMG1_LAST_FLASH_ROW_NUM             (CY_FLASH_NUMBER_ROWS - 1)

/* Flash row size. This depends on the device type.*/
#define PMG1_FLASH_ROW_SIZE                 (CY_FLASH_SIZEOF_ROW)

/*******************************************************************************
* Function definitions
*******************************************************************************/
/**
 * @brief Write data in buffer to flash row at rowNum.
 * @buffer Buffer containing the data to be written to the flash row.
 * @rowNum Row number to be updated.
 */
pmg1_status_t flash_row_write (uint8_t *buffer, uint16_t rowNum);

/**
 * @brief Read data from flash row at row_num into buffer.
 * @buffer Buffer to read the flash data into.
 * @rowNum Flash row number to be read.
 */
int8_t flash_row_read (uint16_t rowNum, uint8_t *buffer);

/**
 * @brief Clear the flash roe at rowNum
 * @rowNum Flash row number to be cleared
 */
pmg1_status_t flash_row_clear (uint16_t rowNum);

/**
 * @brief Check whether flashing mode has been entered.
 */
bool flash_access_get_status (uint8_t modes);

/**
 * @brief Enter flashing mode.
 * @enable Whether to enter/exit flashing mode.
 */
void flash_enter_mode (bool enable);

/**
 * @brief Check whether flashing mode has been entered.
 * @return Returns true if flashing mode has been entered, false otherwise.
 */
bool flash_access_enabled (void);

/**
 * @brief Set limits to the flash rows that can be accessed.
 * @startRow The lowest row number that can be accessed.
 * @lastRow The highest row number that can be accessed.
 * @mdRow Row containing metadata for the alternate firmware.
 * @blLastRow Last bootloader row. Rows above this can be read.
 */
void flash_set_access_limits (uint16_t startRow, uint16_t lastRow,
                              uint16_t mdRow, uint16_t blLastRow);

#endif /* __FLASH_H__ */

/* [] END OF FILE */
