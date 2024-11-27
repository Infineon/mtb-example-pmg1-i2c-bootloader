/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for the I2C Bootloader for ModusToolbox.
*
* Related Document: See README.md
*
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

#include "cy_utils.h"
#include "cy_pdl.h"
#include "cy_hpi.h"
#include "cybsp.h"
#include "config.h"
#include "boot.h"
#include "flash.h"
#include "timer.h"
#include "pmg1_version.h"
#include "pmg1_bsp.h"

/* Device silicon ID */
#define CY_PMG1_SILICON_ID              CY_SILICON_ID

/* Bootloader ram data valid signature. */
#define BL_APP_DATA_VALID_SIG           0x4946u

/* HPI extended version info. */
#define HPI_VERSION_EXT_INFO            0x00000001u

/* Linker symbol used by the cymcuelf tool.*/
#if defined(__ARMCC_VERSION)
__asm
(
    /* flash */
    ".global __cy_memory_0_start    \n"
    ".global __cy_memory_0_length   \n"
    ".global __cy_memory_0_row_size \n"

    /* flash */
#if (defined(CY_DEVICE_SERIES_PMG1S0))
    ".equ __cy_memory_0_start,    0x00000000 \n"
    ".equ __cy_memory_0_length,   0x00010000 \n"
    ".equ __cy_memory_0_row_size, 0x80 \n"
#elif (defined(CY_DEVICE_SERIES_PMG1S1))
    ".equ __cy_memory_0_start,    0x00000000 \n"
    ".equ __cy_memory_0_length,   0x00020000 \n"
    ".equ __cy_memory_0_row_size, 0x100 \n"
#elif (defined(CY_DEVICE_SERIES_PMG1S2))
    ".equ __cy_memory_0_start,    0x00000000 \n"
    ".equ __cy_memory_0_length,   0x00020000 \n"
    ".equ __cy_memory_0_row_size, 0x80 \n"
#else /* (defined(CY_DEVICE_SERIES_PMG1S3)) */
    ".equ __cy_memory_0_start,    0x00000000 \n"
    ".equ __cy_memory_0_length,   0x00040000 \n"
    ".equ __cy_memory_0_row_size, 0x100 \n"
#endif /* (defined(CY_DEVICE_SERIES_PMG1S0)) */
);
#endif /* defined(__ARMCC_VERSION) */

/*******************************************************************************
* Flash fixed offset (0xE0) to store the firmware information.
*******************************************************************************/
/* Composite firmware stack version value.*/
CY_SECTION(".cy_base_version") __USED
const uint32_t glBaseVersion = PMG1_CE_BASE_VERSION;

/* Custom application version. */
CY_SECTION(".cy_app_version") __USED
const uint32_t glAppVersion = 0x00;

/* To store the silicon ID.*/
CY_SECTION(".cy_dev_siliconid") __USED
const uint32_t glPmg1SiliconId = CY_PMG1_SILICON_ID;

/* Reserved bytes for future use. */
CY_SECTION(".cy_fw_reserved") __USED
const uint32_t glReservedBuf[5] = {0};

/*******************************************************************************
* RAM fixed offset as shared memory between bootloader and application.
*******************************************************************************/
/* To store the current boot mode status.*/
#if defined(__ARMCC_VERSION)
CY_SECTION(".bss.cy_boot_run_type") __USED
#else
CY_SECTION(".cy_boot_run_type") __USED
#endif /* defined(__ARMCC_VERSION) */
volatile uint32_t cyBtldrRunType;

/* Stores the signature to validate the RAM data shared from bootloader. */
#if defined(__ARMCC_VERSION)
CY_SECTION(".bss.cy_boot_data_sig") __USED
#else
CY_SECTION(".cy_boot_data_sig") __USED
#endif /* defined(__ARMCC_VERSION) */
volatile uint16_t glBootDataSignature;

/* To share the HPI I2C slave address from bootloader. */
#if defined(__ARMCC_VERSION)
CY_SECTION(".bss.cy_boot_i2c_addr") __USED
#else
CY_SECTION(".cy_boot_i2c_addr") __USED
#endif /* defined(__ARMCC_VERSION) */
volatile uint8_t glHpiSlaveAddr;

/*******************************************************************************
* Global variables.
*******************************************************************************/
/* Boot-wait window related defines and variables.*/
static volatile bool glBootWaitElapsed = false;

/* HPI SCB and interrupt pin configuration.*/
static cy_stc_hpi_context_t glHpiContext;

/* Additional metadata information used by the 'CyMCUElfToo' tool. */
CY_SECTION(".cymeta") __USED
const uint8_t cy_metadata[] = {
    0x00u, 0x02u,
    (CY_PMG1_SILICON_ID>>24)& 0xFF,
    (CY_PMG1_SILICON_ID>>16)& 0xFF,
    ((CY_PMG1_SILICON_ID>>8)& 0xFF),
    (CY_PMG1_SILICON_ID)& 0xFF,
    0x00u, 0x00u,
    0x3Au, 0x0Bu, 0x14u, 0xAEu
};

cy_stc_hpi_hw_config_t glHpiHwConfig =
{
    .scbBase = HPI_I2C_HW,
    .scbPort = HPI_I2C_SCL_PORT,
    .slaveAddr = CY_HPI_ADDR_I2C_CFG_FLOAT,
    .ecIntPort = HPI_EC_INT_PORT,
    .ecIntPin = HPI_EC_INT_PIN
};

/* CYBSP_I2C_SCB_IRQ*/
const cy_stc_sysint_t HPI_SCB_IRQ_CONFIG = {
     .intrSrc = (IRQn_Type) HPI_I2C_IRQ,
     .intrPriority = 3u
 };

/*******************************************************************************
* Function definition
*******************************************************************************/

/* HPI I2C interrupt handler.*/
 void hpi_scb_interrupt_IRQHandler(void)
 {
     /* ISR implementation for I2C*/
     Cy_Hpi_I2cInterruptHandler(&glHpiContext);
 }

/* Timer callback used to identify that boot-wait window has elapsed.*/
static void bl_timer_cb (void)
{
   glBootWaitElapsed = true;
}

/* EC Interrupt status.*/
void hpi_ec_intr_write(bool value)
{
    /* Handler to control the HPI EC interrupt pin.*/
    Cy_GPIO_Write(glHpiHwConfig.ecIntPort, glHpiHwConfig.ecIntPin, value);
}

/* Firmware run type signature.*/
void set_bootloader_run_type(uint32_t runType)
{
    /* Function updates the current firmware run type status.*/
    cyBtldrRunType = runType;
}

/* Flash row to be updated.*/
int8_t hpi_flash_row_write(uint16_t rowNum, uint8_t *data, void *cbk)
{
    /*Write the given data to the specified flash row.*/
    (void)cbk;
    return flash_row_write(data, rowNum);
}

/*  Firmware which needs to be validated.*/
int8_t hpi_boot_validate_fw_cmd(uint8_t fwMode)
{
    /* This function is used to validate the firmware image.*/
    fw_metadata_t *fwMetaLoc = NULL;
    if(fwMode==PMG1_FW_MODE_FWIMAGE_1)
    {
        fwMetaLoc = (fw_metadata_t *)PMG1_FW1_METADATA_ADDR;
    }
    else
    {
        fwMetaLoc = (fw_metadata_t *)PMG1_FW2_METADATA_ADDR;
    }
    return boot_validate_firmware(fwMetaLoc);
}

/*  Enable/Disable Flashing Mode.*/
void hpi_flash_enter_mode(bool isEnable, uint8_t mode, bool dataInPlace)
{
    /*Handle ENTER_FLASHING_MODE Command.*/
    flash_enter_mode(isEnable);
    (void)mode;
    (void)dataInPlace;
}

cy_stc_hpi_app_cbk_t hpiAppCbk =
{
    .ec_intr_write = hpi_ec_intr_write,
    .sys_get_custom_info_addr = NULL,
#if ((SROM_CODE_HPISS_HPI == MODULE_IN_ROM) || (CY_HPI_PD_ENABLE))
    .hpi_is_event_enabled = NULL,
    .app_disable_pd_port = NULL,
#if ((SROM_CODE_HPISS_HPI == MODULE_IN_ROM) || (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP))
    .alt_mode_get_status = NULL,
#endif /* ((SROM_CODE_HPISS_HPI == MODULE_IN_ROM) || (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)) */
    .app_update_sys_pwr_state = NULL,
#if ((SROM_CODE_HPISS_HPI == MODULE_IN_ROM) || (CY_HPI_PD_CMD_ENABLE))
#if ((SROM_CODE_HPISS_HPI == MODULE_IN_ROM) || ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)))
    .set_custom_svid = NULL,
    .set_alt_mode_mask = NULL,
    .app_vdm_layer_reset = NULL,
    .eval_app_alt_mode_cmd = NULL,
    .eval_app_alt_hw_cmd = NULL,
#endif /* ((SROM_CODE_HPISS_HPI == MODULE_IN_ROM) || ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))) */
    .set_custom_host_cap_control = NULL,
    .app_set_custom_pid = NULL,
    .i2cm_gen_i2c_tunnel_cmd = NULL,
    .switch_vddd_supply = NULL,
#if ((SROM_CODE_HPISS_HPI == MODULE_IN_ROM) || (CY_HPI_VDM_QUERY_SUPPORTED))
    *.vdm_get_disc_id_resp = NULL,
    *.vdm_get_disc_svid_resp = NULL,
#endif /* ((SROM_CODE_HPISS_HPI == MODULE_IN_ROM) || (CY_HPI_VDM_QUERY_SUPPORTED)) */
    .app_update_bc_src_support = NULL,
#if ((SROM_CODE_HPISS_HPI == MODULE_IN_ROM) || (CY_HPI_VBUS_C_CTRL_ENABLE))
    .psnk_set_vbus_cfet_on_ctrl = NULL,
#endif /* ((SROM_CODE_HPISS_HPI == MODULE_IN_ROM) || (CY_HPI_VBUS_C_CTRL_ENABLE)) */
    .hpi_vconn_enable = NULL,
    .hpi_vconn_disable = NULL,
#if ((SROM_CODE_HPISS_HPI == MODULE_IN_ROM) || (CY_HPI_RW_PD_RESP_MSG_DATA))
    .hpi_rw_pd_resp_data = NULL,
#endif /* ((SROM_CODE_HPISS_HPI == MODULE_IN_ROM) || (CY_HPI_RW_PD_RESP_MSG_DATA)) */
#endif /* ((SROM_CODE_HPISS_HPI == MODULE_IN_ROM) || (CY_HPI_PD_CMD_ENABLE)) */
    .vbus_get_live_current = NULL,
#endif /* ((SROM_CODE_HPISS_HPI == MODULE_IN_ROM) || (CY_HPI_PD_ENABLE)) */
    .set_bootloader_run_type = set_bootloader_run_type,
#if ((SROM_CODE_HPISS_HPI == MODULE_IN_ROM) || (!CY_HPI_BOOT_ENABLE))
.hpi_boot_validate_fw = NULL,
#endif /* ((SROM_CODE_HPISS_HPI == MODULE_IN_ROM) || (!CY_HPI_BOOT_ENABLE)) */
    .hpi_boot_validate_fw_cmd = hpi_boot_validate_fw_cmd,
#if ((SROM_CODE_HPISS_HPI == MODULE_IN_ROM) || (!CY_HPI_BOOT_ENABLE))
.hpi_sys_get_device_mode = NULL,
#endif /* ((SROM_CODE_HPISS_HPI == MODULE_IN_ROM) || (!CY_HPI_BOOT_ENABLE)) */
#if ((SROM_CODE_HPISS_HPI == MODULE_IN_ROM) || (CY_HPI_FLASH_RW_ENABLE))
    .hpi_flash_row_write = hpi_flash_row_write,
    .hpi_flash_row_read = flash_row_read,
    .hpi_flash_access_get_status = flash_access_get_status,
    .hpi_flash_enter_mode = hpi_flash_enter_mode,
#endif /* ((SROM_CODE_HPISS_HPI == MODULE_IN_ROM) || (CY_HPI_FLASH_RW_ENABLE)) */
#if ((SROM_CODE_HPISS_HPI == MODULE_IN_ROM) || (CCG_UCSI_ENABLE))
.ucsi_notify = NULL,
.ucsi_reg_space_write_handler = NULL,
.ucsi_handle_hpi_commands = NULL,
.hpi_update_ucsi_reg_space = NULL,
#endif /* ((SROM_CODE_HPISS_HPI == MODULE_IN_ROM) || (CCG_UCSI_ENABLE)) */
    .hpi_dev_wr_handler_ext = NULL,
    .hpi_port_wr_handler_ext = NULL,

};

static void update_hpi_regs (void)
{
    uint8_t mode, reason;
    uint32_t fw1Ver, fw2Ver;
    uint16_t fw1Loc, fw2Loc;
    fw_metadata_t *fw1Md, *fw2Md;
    uint8_t invalidVer[8] = {0};

    /* HPI Version v2, selected flash row size, selected no. of ports, boot-loader running.*/
    mode   = 0x80 | (PMG1_FLASH_ROW_SIZE_DEV_MODE_VAL << 4) | ((NO_OF_TYPEC_PORTS - 1) << 2);

    reason = boot_mode_get_reason ();

    /* Update device mode and boot reason HPI registers.*/
    Cy_Hpi_SetModeRegs(&glHpiContext, mode, reason);

    /* Calculate the firmware1 version and address from the firmware metadata.*/
    if ((reason & 0x04) != 0)
    {
        fw1Ver = (uint32_t)invalidVer;
        fw1Loc = PMG1_LAST_FLASH_ROW_NUM + 1;
    }
    else
    {
        fw1Md  = (fw_metadata_t *)PMG1_FW1_METADATA_ADDR;
        fw1Ver = (((uint32_t)fw1Md->bootLastRow + 1) << PMG1_FLASH_ROW_SHIFT_NUM) + PMG1_FW_VERSION_OFFSET;
        fw1Loc = fw1Md->bootLastRow + 1;
    }

    /* Calculate the firmware2 version and address from the firmware metadata.*/
    if ((reason & 0x08) != 0)
    {
        fw2Ver = (uint32_t)invalidVer;
        fw2Loc = PMG1_LAST_FLASH_ROW_NUM + 1;
    }
    else
    {
        fw2Md  = (fw_metadata_t *)PMG1_FW2_METADATA_ADDR;
        fw2Ver = (((uint32_t)fw2Md->bootLastRow + 1) << PMG1_FLASH_ROW_SHIFT_NUM) + PMG1_FW_VERSION_OFFSET;
        fw2Loc = fw2Md->bootLastRow + 1;
    }

    /* Update version information in the HPI registers.*/
    Cy_Hpi_UpdateVersions(
            &glHpiContext,
            (uint8_t *)PMG1_BL_VERSION_ADDR,
            (uint8_t *)fw1Ver,
            (uint8_t *)fw2Ver);

    /* Update version extended register. */
    Cy_Hpi_SetHpiVersionExt(&glHpiContext, HPI_VERSION_EXT_INFO);

    /* Update firmware location registers.*/
    Cy_Hpi_UpdateFwLocations(&glHpiContext, fw1Loc, fw2Loc);
    /* Set flash parameters.*/
    Cy_Hpi_SetFlashParams(&glHpiContext, PMG1_FLASH_SIZE, PMG1_FLASH_ROW_SIZE,
                          PMG1_LAST_FLASH_ROW_NUM + 1, PMG1_BOOT_LOADER_LAST_ROW);

    /* Update Silicon ID. */
    Cy_Hpi_UpdateRegs(&glHpiContext,
                      (uint8_t)CY_HPI_REG_SECTION_DEV, 0x02,
                      ((uint8_t *) &glPmg1SiliconId) + 2, 0x02);
}

static void get_hpi_slave_addr(void)
{
    uint8_t slaveAddr = CY_HPI_ADDR_I2C_CFG_FLOAT;

    /* Check if IO is driven low.*/
    Cy_GPIO_SetDrivemode(HPI_ADDR_CFG_PORT, HPI_ADDR_CFG_PIN, CY_GPIO_DM_PULLUP);
    Cy_GPIO_Write(HPI_ADDR_CFG_PORT, HPI_ADDR_CFG_PIN, 1);
    Cy_SysLib_DelayUs(5);
    if (Cy_GPIO_Read(HPI_ADDR_CFG_PORT, HPI_ADDR_CFG_PIN) == 0)
    {
        slaveAddr = CY_HPI_ADDR_I2C_CFG_LOW;
    }
    else
    {
        /* Check if IO is driven high.*/
        Cy_GPIO_SetDrivemode(HPI_ADDR_CFG_PORT, HPI_ADDR_CFG_PIN, CY_GPIO_DM_PULLDOWN);
        Cy_GPIO_Write(HPI_ADDR_CFG_PORT, HPI_ADDR_CFG_PIN, 0);
        Cy_SysLib_DelayUs(5);
        if (Cy_GPIO_Read(HPI_ADDR_CFG_PORT, HPI_ADDR_CFG_PIN) != 0)
        {
            slaveAddr = CY_HPI_ADDR_I2C_CFG_HIGH;
        }
    }
    /* Disable the pull up/pull down on IO.*/
    Cy_GPIO_SetDrivemode(HPI_ADDR_CFG_PORT, HPI_ADDR_CFG_PIN, CY_GPIO_DM_HIGHZ);

    /* Update the slave address.*/
    glHpiHwConfig.slaveAddr = slaveAddr;
    glHpiSlaveAddr = slaveAddr;
}

int main(void)
{
    uint32_t wait;

    /* Initialize the device and board peripherals.*/
    pmg1_bsp_init();

    /*Enable the systick interrupt. This is used by the soft timer.*/
    NVIC_EnableIRQ(SysTick_IRQn);

    /* Initialize the soft timer module.*/
    timer_init();

    /* Enable global interrupts.*/
    __enable_irq();

    /* Update the HPI slave address so that it can be passed to application.*/
    get_hpi_slave_addr();

    /* If we have a valid firmware binary, load it.*/
    if (boot_start () == true)
    {
        glBootDataSignature = BL_APP_DATA_VALID_SIG;
        wait = boot_get_wait_time ();
        if (wait == 0)
        {
            boot_jump_to_fw ();
        }
        else
        {
            /* Make sure boot-wait elapsed flag is cleared.*/
            glBootWaitElapsed = false;

            /* We need a timer to wait for the boot-wait timeout period.*/
            timer_start (wait, bl_timer_cb);
        }
    }

    /* Initialize the HPI interface.*/
    Cy_Hpi_Init(&glHpiContext, &glHpiHwConfig, &hpiAppCbk, NULL, NULL, 0);
    Cy_SysInt_Init(&HPI_SCB_IRQ_CONFIG, &hpi_scb_interrupt_IRQHandler);
    NVIC_EnableIRQ((IRQn_Type) HPI_SCB_IRQ_CONFIG.intrSrc);

    /* Update the HPI registers.*/
    update_hpi_regs ();

    /* Send a reset complete event to the EC.*/
    Cy_Hpi_RegEnqueueEvent(&glHpiContext, CY_HPI_REG_SECTION_DEV, CY_HPI_EVENT_RESET_COMPLETE, 0, NULL);

    /* Set the flash access boundaries so that the boot-loader itself cannot be overwritten.*/
    flash_set_access_limits (PMG1_BOOT_LOADER_LAST_ROW + 1, PMG1_LAST_FLASH_ROW_NUM,
                             PMG1_LAST_FLASH_ROW_NUM, PMG1_BOOT_LOADER_LAST_ROW);

    for (;;)
    {
        /* Handle any pending HPI commands.*/
        Cy_Hpi_Task(&glHpiContext);

        /* If flashing mode is entered, disable the timer and stay in boot-loader mode.*/
        if (flash_access_enabled ())
        {
            timer_stop ();
            glBootWaitElapsed = false;
        }

        /* Jump to the selected firmware once the boot-wait window has elapsed.*/
        if (glBootWaitElapsed)
        {
            boot_jump_to_fw();
        }
    }
}

void Cy_OnResetUser(void)
{
    /* Reset handler implementation on user, this function will be executed first on reset.*/
    if (Cy_SysLib_GetResetReason() != CY_SYSLIB_RESET_SOFT)
    {
        /*If there is a soft reset then stay in boot-loader mode. */
        cyBtldrRunType = PMG1_BOOT_TYPE_STAY_IN_BOOT;
    }
    else
    {
        if (cyBtldrRunType == PMG1_BOOT_TYPE_START_APP)
        {
            /*If run type is start application, clear the run type and jump to application. */          
            cyBtldrRunType = PMG1_BOOT_TYPE_STAY_IN_BOOT;
            (void) boot_jump_to_app();
        }
    }
}
/* [] END OF FILE */
