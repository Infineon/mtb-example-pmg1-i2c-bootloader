/******************************************************************************
* File Name: config.h
*
* Description: This header file defines the application configuration.
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

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "cy_flash.h"

/* Last Boot loader row. This field should be changed only when there is a
 * project level change, and should be coupled with a change in the linker settings
 * and boot-loadable placement settings for the firmware application.
 */
#if (CY_FLASH_SIZEOF_ROW == 128)
#define PMG1_BOOT_LOADER_LAST_ROW        (0x37)
#elif (CY_FLASH_SIZEOF_ROW == 256)
#define PMG1_BOOT_LOADER_LAST_ROW        (0x1B)
#else
#error "Selected device has unsupported flash row size."
#endif /* CY_FLASH_SIZEOF_ROW */

/* Size of firmware metadata.*/
#define PMG1_FW_METADATA_SIZE            (128)

/* Metadata location for FW1.*/
#define PMG1_FW1_METADATA_ROW            (PMG1_LAST_FLASH_ROW_NUM)
#define PMG1_FW1_METADATA_ADDR           (((PMG1_FW1_METADATA_ROW + 1) << PMG1_FLASH_ROW_SHIFT_NUM) - PMG1_FW_METADATA_SIZE)

/* Metadata location for FW2.*/
#define PMG1_FW2_METADATA_ROW            (PMG1_LAST_FLASH_ROW_NUM - 1)
#define PMG1_FW2_METADATA_ADDR           (((PMG1_FW2_METADATA_ROW + 1) << PMG1_FLASH_ROW_SHIFT_NUM) - PMG1_FW_METADATA_SIZE)

/* Firmware Version Information is located at a fixed offset of 0xE0 from start of firmware location.*/
#define PMG1_FW_VERSION_OFFSET           (0xE0)

/* Since BL starts at address 0, BL version address is same as version offset.*/
#define PMG1_BL_VERSION_ADDR             (PMG1_FW_VERSION_OFFSET)


#endif /* _CONFIG_H_ */

/* End of file */
