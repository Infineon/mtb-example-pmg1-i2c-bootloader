/******************************************************************************
* File Name: boot.h
*
* Description: This is boot header file for the PMG1
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

#ifndef __BOOT_H__
#define __BOOT_H__

#include <stdbool.h>
#include "config.h"
#include "status.h"
#include "flash.h"

/*****************************************************************************
* MACRO Definition
*****************************************************************************/

/* Signature used for firmware to indicate boot mode request.*/
#define PMG1_BOOT_MODE_RQT_SIG           (0x424C)

/* Signature used to indicate boot FW1 request.*/
#define PMG1_FW1_BOOT_RQT_SIG            (0x4231)

/* Signature used to indicate boot FW2 request.*/
#define PMG1_FW2_BOOT_RQT_SIG            (0x4232)

/* Firmware image is valid*/
#define PMG1_FW_VALID                    (0)

/* Firmware image is invalid*/
#define PMG1_FW_INVALID                  (1)

/* Firmware metadata valid signature: "IF" */
#define PMG1_FW_METADATA_VALID_SIG       (0x4946)

/* Boot type stay in boot-loader.*/
#define PMG1_BOOT_TYPE_STAY_IN_BOOT     (0x00U)

/* Boot type start application flag.*/
#define PMG1_BOOT_TYPE_START_APP        (0x80U)

/* Firmware boot sequence number offset.*/
#define PMG1_FW_METADATA_BOOTSEQ_OFFSET  (0x14)

/* No delay for PMG1 boot-loader: 0 ms */
#define PMG1_BL_WAIT_NO_DELAY            (0)

/* Default boot-wait window for PMG1 boot-loader: 50 ms */
#define PMG1_BL_WAIT_DEFAULT             (50)

/* Minimum boot-wait window duration supported: 20 ms*/
#define PMG1_BL_WAIT_MINUMUM             (20)

/* Maximum boot-wait window duration supported: 1000 ms*/
#define PMG1_BL_WAIT_MAXIMUM             (1000)

/* FW metadata App ID value requesting default boot-wait window.*/
#define PMG1_FWMETA_WAIT_TIME_DEF       (0xFFFFu)

/* FW metadata App ID value requesting boot-wait of zero.*/
#define PMG1_FWMETA_WAIT_TIME_0         (0x4359)

/*****************************************************************************
* Enumerated Data Definition
*****************************************************************************/

/**
 * @typedef pmg1_fw_mode_t
 * @Enumeration of PMG1 firmware modes.
 */
typedef enum
{
    PMG1_FW_MODE_BOOTLOADER = 0,     /**< Bootloader mode.*/
    PMG1_FW_MODE_FWIMAGE_1,          /**< Firmware Image #1*/
    PMG1_FW_MODE_FWIMAGE_2,          /**< Firmware Image #2*/
    PMG1_FW_MODE_INVALID             /**< Invalid value.*/
} pmg1_fw_mode_t;

/*****************************************************************************
* Data Struct Definition
*****************************************************************************/

/**
 * @typedef fw_metadata_t
 * @brief Struct to hold USB-PD FW METADATA
 */
typedef struct __attribute__((__packed__))
{
    uint32_t appFwStart;            /**< Offset 00: Firmware Start Address. */
    uint32_t appFwSize;             /**< Offset 04: Firmware Size */
    uint16_t bootWaitTime;          /**< Offset 08: Boot wait time */
    uint16_t bootLastRow;           /**< Offset 0A: Last Flash row of Bootloader or previous firmware. */
    uint32_t reserved1[2];          /**< Offset 0C: Reserved. */
    uint32_t bootSeq;               /**< Offset 14: Boot sequence number field. Boot-loader will load the valid
                                         FW copy that has the higher sequence number associated with it. */
    uint32_t reserved2[15];         /**< Offset 18: Reserved. */
    uint16_t metadataVersion;       /**< Offset 54: Version of the metadata structure. */
    uint16_t metadataValid;         /**< Offset 56: Metadata Valid field. Valid if contains "IF". */
    uint32_t fwCrc32;               /**< Offset 58: Verify Fw CRC32 checksum */
    uint32_t reserved3[8];          /**< Offset 5C: Reserved. */
    uint32_t reserved;              /**< Offset 7C: Reserved for Metadata CRC32 checksum. */
} fw_metadata_t;

/**
 * @typedef bootmode_reason_t
 * @brief Structure to hold reason for boot mode.
 */
typedef union
{
    uint8_t val;
    struct fw_mode_reason
    {
        uint8_t bootModeRequest  : 1;      /**< Boot mode request made by FW. */
        uint8_t reserved1        : 1;      /**< Reserved for later use. */
        uint8_t fw1Invalid       : 1;      /**< FW1 image invalid: 0=Valid, 1=Invalid. */
        uint8_t fw2Invalid       : 1;      /**< FW2 image invalid: 0=Valid, 1=Invalid. */
        uint8_t reserved2        : 4;      /**< Reserved for later use. */
    } status;
} fw_img_status_t;


/*****************************************************************************
* Global Function Declaration
*****************************************************************************/

#if PMG1_BOOTLOAD_ENABLE
/*
 * Starts boot-loading operation. Validates if any valid FW image
 * is present. If yes, this API returns true. If not return value
 * is false. The caller is responsible to handle both cases and
 * decide whether to switch control to FW or not.
 * @param NONE
 * @return bool
 */
bool boot_start (void);
#endif /* PMG1_BOOTLOAD_ENABLE */

/**
 * @brief Returns the boot-wait delay configured for the application.
 * @return Boot-wait delay in milliseconds.
 */
uint16_t boot_get_wait_time (void);

/**
 * @brief Schedules the FW app and undergoes a RESET.
 * @param NONE
 * @return NONE
 */
void boot_jump_to_fw (void);

/**
 * Returns Bit map containing the reason for boot mode.
 * @param NONE
 * @return uint8_t Boot mode reason bitmap.
 */
uint8_t boot_mode_get_reason (void);

/**
 * @brief Validate a firmware binary.
 * @md_p Pointer to metadata regarding firmware to be validated.
 * @return pmg1_status_t PMG1_STAT_SUCCESS if valid, PMG1_STAT_FAILURE otherwise.
 */
pmg1_status_t boot_validate_firmware (fw_metadata_t *mdP);

/**
 * @brief Get the boot sequence number value for the specified firmware image.
 * @fwid ID of the firmware image to be queried.
 * @return Boot sequence number value.
 */
uint32_t boot_get_boot_seq (uint8_t fwId);

/**
 * @brief Function gets the active image meta data and jumps to the application.
 */
void boot_jump_to_app(void);

#endif /* __BOOT_H__ */

/* [] END OF FILE */
