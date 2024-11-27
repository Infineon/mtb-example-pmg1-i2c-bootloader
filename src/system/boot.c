/******************************************************************************
* File Name: boot.c
*
* Description: This is boot source file for the PMG1
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
#include "cy_utils.h"
#include "cy_pdutils.h"
#include "config.h"
#include "flash.h"
#include "boot.h"

/* A number of uint32_t elements in the CRC32 table.*/
#define CRC_TABLE_SIZE                      (16U)
#define CRC_INIT                            (0xFFFFFFFFU)
#define NIBBLE_POS                          (4U)
#define NIBBLE_MSK                          (0xFU)

extern volatile uint32_t cyBtldrRunType;

#if defined(__ARMCC_VERSION)
CY_SECTION(".bss.cy_boot_img_status") __USED
#else
CY_SECTION(".cy_boot_img_status") __USED
#endif /* defined(__ARMCC_VERSION) */
volatile fw_img_status_t gl_img_status;

/* Variable representing the current firmware mode.*/
pmg1_fw_mode_t glActiveFw = PMG1_FW_MODE_INVALID;

#if PMG1_BOOTLOAD_ENABLE

/* Boot-wait duration specified by firmware metadata.*/
static volatile uint16_t glBootWaitDelay = PMG1_BL_WAIT_DEFAULT;
#endif /* PMG1_BOOTLOAD_ENABLE */

/* Pointer to function that is used to jump into address.*/
typedef void (*cy_fn_jump_ptr_t)(void);

uint32_t calculate_crc32(const uint8_t *address, uint32_t length)
{
    /* Contains generated values to calculate CRC-32C by 4 bits per iteration.
    CRC-32C is computed based on the polynomial (0x1EDC6F41).*/
    static const uint32_t crcTable[CRC_TABLE_SIZE] =
    {
        0x00000000U, 0x105ec76fU, 0x20bd8edeU, 0x30e349b1U,
        0x417b1dbcU, 0x5125dad3U, 0x61c69362U, 0x7198540dU,
        0x82f63b78U, 0x92a8fc17U, 0xa24bb5a6U, 0xb21572c9U,
        0xc38d26c4U, 0xd3d3e1abU, 0xe330a81aU, 0xf36e6f75U,
    };
    uint32_t crc = CRC_INIT;
    if (length != 0U)
    {
        do
        {
            crc = crc ^ *address;
            crc = (crc >> NIBBLE_POS) ^ crcTable[crc & NIBBLE_MSK];
            crc = (crc >> NIBBLE_POS) ^ crcTable[crc & NIBBLE_MSK];
            --length;
            ++address;
        } while (length != 0U);
    }
    return (~crc);
}

/* Return the boot-wait setting to the user code.*/
uint16_t boot_get_wait_time (void)
{
    return (glBootWaitDelay);
}

/* Validate a firmware binary.*/
pmg1_status_t boot_validate_firmware (fw_metadata_t *mdP)
{
    if(mdP==NULL)
    {
        return PMG1_STAT_FAILURE;
    }
    else
    {
        pmg1_status_t      status = PMG1_STAT_SUCCESS;
        uint32_t          *appFwStart;
        uint32_t          appFwSize;

        appFwStart = (uint32_t *)mdP->appFwStart;
        appFwSize  = mdP->appFwSize;

        /* Validate:
           1) FW signature
           2) FW entry and size
           3) FW checksum
         */
        if (
            (mdP->metadataValid != PMG1_FW_METADATA_VALID_SIG)
            ||(((uint32_t)appFwStart + appFwSize) >= PMG1_FLASH_SIZE)
            ||(appFwSize == 0)
            ||(mdP->fwCrc32 != calculate_crc32((uint8_t *)appFwStart, appFwSize))
           )
        {
            status = PMG1_STAT_FAILURE;
        }
        return status;
    }
}

#if PMG1_BOOTLOAD_ENABLE
static void boot_set_wait_timeout (fw_metadata_t *mdP)
{
    /* Check for boot-wait option.*/
    if (mdP->bootWaitTime == PMG1_FWMETA_WAIT_TIME_0)
    {
        glBootWaitDelay = PMG1_BL_WAIT_NO_DELAY;
    }
    else
    {
        if (mdP->bootWaitTime != PMG1_FWMETA_WAIT_TIME_DEF)
        {
            /* Get the boot-wait delay from metadata, applying the MIN and MAX limits.*/
            glBootWaitDelay = CY_PDUTILS_GET_MAX (PMG1_BL_WAIT_MAXIMUM, CY_PDUTILS_GET_MIN (PMG1_BL_WAIT_MINUMUM, mdP->bootWaitTime));
        }
    }
}

bool boot_start (void)
{
    fw_metadata_t *md1P=NULL;
    fw_metadata_t *md2P=NULL;
    fw_metadata_t *mdP=NULL;
    pmg1_fw_mode_t activeFw;
    bool    bootFw1 = false;
    bool    bootFw2 = false;

    /* Clear the reason for boot mode. */
    gl_img_status.val = 0;

    /* Check the two firmware binaries for validity.*/
    if (boot_validate_firmware ((fw_metadata_t *)PMG1_FW1_METADATA_ADDR) != PMG1_STAT_SUCCESS)
    {
        gl_img_status.status.fw1Invalid  = PMG1_FW_INVALID;
    }
    if (boot_validate_firmware ((fw_metadata_t *)PMG1_FW2_METADATA_ADDR) != PMG1_STAT_SUCCESS)
    {
        gl_img_status.status.fw2Invalid = PMG1_FW_INVALID;
    }

    /* Check for the boot mode request.*/
    /* NOTE: glBootloaderRunType is Bootloader component provided variable.
     * It is used to store the jump signature. Check the lower two bytes
     * for signature. */
    if ((cyBtldrRunType & 0xFFFF) == PMG1_BOOT_MODE_RQT_SIG)
    {
        /* FW has made a request to stay in boot mode. Return
         * from here after clearing the variable.*/
        cyBtldrRunType = PMG1_BOOT_TYPE_STAY_IN_BOOT;
        /* Set the reason for boot mode.*/
        gl_img_status.status.bootModeRequest = 1;
        return false;
    }

    /* Check if we have been asked to boot FW1 or FW2 specifically.*/
    if ((cyBtldrRunType & 0xFFFF) == PMG1_FW1_BOOT_RQT_SIG)
    {
        bootFw1 = true;
    }
    if ((cyBtldrRunType & 0xFFFF) == PMG1_FW2_BOOT_RQT_SIG)
    {
        bootFw2 = true;
    }

    /* Check image pointer values and try to boot the newer image.*/
    md1P = (fw_metadata_t *)PMG1_FW1_METADATA_ADDR;
    md2P = (fw_metadata_t *)PMG1_FW2_METADATA_ADDR;

    /* If we have been specifically asked to boot FW2, do that.
       Otherwise, if we have not been specifically asked to boot FW1; choose the binary with
       greater sequence number.
     */
    if (!gl_img_status.status.fw2Invalid)
    {
        /* FW2 is valid.
           We can boot this if:
           1. We have been asked to boot FW2.
           2. FW1 is not valid.
           3. FW2 is newer than FW1, and we have not been asked to boot FW1.
         */
        if ((bootFw2) || (gl_img_status.status.fw1Invalid) || ((!bootFw1) && (md2P->bootSeq >= md1P->bootSeq)))
        {
            mdP = md2P;
            activeFw  =  PMG1_FW_MODE_FWIMAGE_2;
        }
        else
        {
            mdP = md1P;
            activeFw  = PMG1_FW_MODE_FWIMAGE_1;
        }
    }
    else
    {
        /* FW2 is invalid.
           Load FW1 if it is valid.
         */
        if (!gl_img_status.status.fw1Invalid)
        {
            mdP = md1P;
            activeFw  = PMG1_FW_MODE_FWIMAGE_1;
        }
    }

    if (mdP != NULL)
    {
        /* If we are in the middle of a jump-to-alt-fw command, do not provide the boot wait window.*/
        if ((bootFw1) || (bootFw2))
            glBootWaitDelay = PMG1_BL_WAIT_NO_DELAY;
        else
            boot_set_wait_timeout (mdP);

        glActiveFw = activeFw;
        return true;
    }

    /* Boot failed. Continue in boot mode.*/
    return false;
}

/* Schedule the FW and undergo a reset.*/
void boot_jump_to_fw (void)
{
    cyBtldrRunType = PMG1_BOOT_TYPE_START_APP;
    Cy_SysLib_ClearResetReason();
    NVIC_SystemReset();
}
#endif /* PMG1_BOOTLOAD_ENABLE */

/* Return the reason for boot mode.*/
uint8_t boot_mode_get_reason (void)
{
    return (gl_img_status.val);
}

/* Get the boot sequence number value for the specified firmware image.*/
uint32_t boot_get_boot_seq (uint8_t fwId)
{
    fw_metadata_t *mdP;

    if (fwId == PMG1_FW_MODE_FWIMAGE_1)
    {
        mdP = (fw_metadata_t *)PMG1_FW1_METADATA_ADDR;
    }
    else
    {
        mdP = (fw_metadata_t *)PMG1_FW2_METADATA_ADDR;
    }

    if (boot_validate_firmware (mdP) == PMG1_STAT_SUCCESS)
    {
        return (mdP->bootSeq);
    }

    return 0;
}

static void SwitchToApp(uint32_t stackPointer, uint32_t address)
{
    __set_MSP(stackPointer);
    ((cy_fn_jump_ptr_t) address) ();
    /* This function does not return.*/
    for(;;)
    {
    }
}

/* Function gets the active image meta data and jumps to the application.*/
void boot_jump_to_app(void)
{
    if(glActiveFw!=PMG1_FW_MODE_INVALID)
    {
        fw_metadata_t *mdP = (fw_metadata_t *)PMG1_FW1_METADATA_ADDR;

        if(glActiveFw == PMG1_FW_MODE_FWIMAGE_2)
        {
            mdP = (fw_metadata_t *)PMG1_FW2_METADATA_ADDR;
        }

        uint32_t fwStart = mdP->appFwStart;
        /* The Stack Pointer of the app to switch to.*/
        uint32_t stackPointer = ((uint32_t *)fwStart)[0];
        /* Reset_Handler() address */
        uint32_t resetHandler = ((uint32_t *)fwStart)[1];
        SwitchToApp(stackPointer, resetHandler);
    }
}

/* [] END OF FILE */
