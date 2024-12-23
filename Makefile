################################################################################
# \file Makefile
# \version 1.0
#
# \brief
# Top-level application make file.
#
################################################################################
# \copyright
# $ Copyright 2024 Cypress Semiconductor Apache2 $
################################################################################


################################################################################
# Basic Configuration
################################################################################

# Type of ModusToolbox Makefile Options include:
#
# COMBINED    -- Top Level Makefile usually for single standalone application
# APPLICATION -- Top Level Makefile usually for multi project application
# PROJECT     -- Project Makefile under Application
#
MTB_TYPE=COMBINED

# Target board/hardware (BSP).
# To change the target, it is recommended to use the Library manager
# ('make modlibs' from command line), which will also update Eclipse IDE launch
# configurations. If TARGET is manually edited, ensure TARGET_<BSP>.mtb with a
# valid URL exists in the application, run 'make getlibs' to fetch BSP contents
# and update or regenerate launch configurations for your IDE.
TARGET=PMG1-CY7110

# Name of application (used to derive name of final linked file).
# 
# If APPNAME is edited, ensure to update or regenerate launch
# configurations for your IDE.
APPNAME=mtb-example-pmg1-i2c-bootloader

# Name of toolchain to use. Options include:
#
# GCC_ARM -- GCC provided with ModusToolbox IDE
# ARM     -- ARM Compiler (must be installed separately)
# IAR     -- IAR Compiler (must be installed separately)
#
# See also: CY_COMPILER_PATH below
TOOLCHAIN=GCC_ARM

# Default build configuration. Options include:
#
# Debug -- build with minimal optimizations, focus on debugging.
# Release -- build with full optimizations
# Custom -- build with custom configuration, set the optimization flag in CFLAGS
#
# If CONFIG is manually edited, ensure to update or regenerate launch configurations
# for your IDE.
CONFIG=Custom

# If set to "true" or "1", display full command-lines when building.
VERBOSE=


################################################################################
# Advanced Configuration
################################################################################

# Enable optional code that is ordinarily disabled by default.
#
# Available components depend on the specific targeted hardware and firmware
# in use. In general, if you have
#
#    COMPONENTS=foo bar
#
# ... then code in directories named COMPONENT_foo and COMPONENT_bar will be
# added to the build
#
COMPONENTS=HPI_SLAVE_BOOT

# Like COMPONENTS, but disable optional code that was enabled by default.
DISABLE_COMPONENTS=

# By default the build system automatically looks in the Makefile's directory
# tree for source code and builds it. The SOURCES variable can be used to
# manually add source code to the build process from a location not searched
# by default, or otherwise not found by the build system.
SOURCES=

# Like SOURCES, but for include directories. Value should be paths to
# directories (without a leading -I).
INCLUDES=

# Add additional defines to the build process (without a leading -D).
# Enabled PD revision 3.0 support, VBus OV Fault Protection and Deep Sleep mode in idle states.

# SYS_DEEPSLEEP_ENABLE Enable deep sleep for power saving.
# PMG1_BOOTLOAD_ENABLE Enable bootloader(Should be disabled in application).
# CY_HPI_PD_ENABLE Enable PD support in HPI.
# CY_HPI_PD_CMD_ENABLE Enable PD command support in HPI library.
# CY_HPI_BB_ENABLE Billboard is disabled.
# CY_HPI_LEGACY_DUAL_APP_EN Enable dual application.
# CY_HPI_BOOT_ENABLE Bootloader with HPI interface enable.
# CY_HPI_FLASH_RW_ENABLE Enable flash read and write.
# CY_HPI_PD_INTR_STATUS_ENABLE PD port status(in form of interrupt status bits) is disabled.
# CY_HPI_VDM_QUERY_SUPPORTED Vendor Defined Message response query is diabled.
# HPI_DEBUG_COMMAND_EN debug command is disabled.
# CY_PD_EPR_ENABLE Extended Power Range is disabled.
# CCG_UCSI_ENABLE UCSI interface is disabled.
# CY_USE_CONFIG_TABLE Configuration table is disabled.
DEFINES+=SYS_DEEPSLEEP_ENABLE=0 PMG1_BOOTLOAD_ENABLE=1\
		 CY_HPI_PD_ENABLE=0 CY_HPI_PD_CMD_ENABLE=0 \
		 CY_HPI_BB_ENABLE=0 CY_HPI_LEGACY_DUAL_APP_EN=1 \
		 CY_HPI_BOOT_ENABLE=1 CY_HPI_FLASH_RW_ENABLE=1 \
		 CY_HPI_PD_INTR_STATUS_ENABLE=0 CY_HPI_VDM_QUERY_SUPPORTED=0 \
		 HPI_DEBUG_COMMAND_EN=0 CY_PD_EPR_ENABLE=0 \
		 CCG_UCSI_ENABLE=0 CY_USE_CONFIG_TABLE=0


DEFINES+=NDEBUG

# Select softfp or hardfp floating point. Default is softfp.
VFP_SELECT=

# Additional / custom C compiler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
ifeq ($(CONFIG), Custom)
ifeq ($(TOOLCHAIN), ARM)
CFLAGS=-flto -Os
else
ifeq ($(TOOLCHAIN), IAR)
CFLAGS=-Ohz
else
CFLAGS=-flto -Os
endif #IAR
endif #ARM
else
CFLAGS=
endif #CONFIG

# Additional / custom C++ compiler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
CXXFLAGS=

# Additional / custom assembler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
ASFLAGS=

# Additional / custom linker flags.
ifeq ($(CONFIG), Custom)
ifeq ($(TOOLCHAIN), ARM)
LDFLAGS=--lto
else
ifeq ($(TOOLCHAIN), IAR)
LDFLAGS=
else
LDFLAGS=
endif #IAR
endif #ARM
else
LDFLAGS=
endif #CONFIG

# Additional / custom libraries to link in to the application.
LDLIBS=

# Path to the linker script to use (if empty, use the default linker script).
ifeq ($(CONFIG), Custom)
ifeq ($(TOOLCHAIN), ARM)
LINKER_EXTN=$(MTB_TOOLCHAIN_ARM__SUFFIX_LS)
else
ifeq ($(TOOLCHAIN), IAR)
LINKER_EXTN=$(MTB_TOOLCHAIN_IAR__SUFFIX_LS)
else
LINKER_EXTN=$(MTB_TOOLCHAIN_GCC_ARM__SUFFIX_LS)
endif #IAR
endif #ARM
LINKER_SCRIPT=$(MTB_TOOLS__TARGET_DIR)/COMPONENT_$(MTB_RECIPE__CORE)/TOOLCHAIN_$(TOOLCHAIN)/pmg1_linker.$(LINKER_EXTN)
else
LINKER_SCRIPT=
endif #CONFIG

# Custom pre-build commands to run.
PREBUILD=

# Custom post-build commands to run.
POSTBUILD=

################################################################################
# Paths
################################################################################

# Relative path to the project directory (default is the Makefile's directory).
#
# This controls where automatic source code discovery looks for code.
CY_APP_PATH=

# Relative path to the shared repo location.
#
# All .mtb files have the format, <URI>#<COMMIT>#<LOCATION>. If the <LOCATION> field
# begins with $$ASSET_REPO$$, then the repo is deposited in the path specified by
# the CY_GETLIBS_SHARED_PATH variable. The default location is one directory level
# above the current app directory.
# This is used with CY_GETLIBS_SHARED_NAME variable, which specifies the directory name.
CY_GETLIBS_SHARED_PATH=../

# Directory name of the shared repo location.
#
CY_GETLIBS_SHARED_NAME=mtb_shared

# Absolute path to the compiler's "bin" directory.
#
# The default depends on the selected TOOLCHAIN (GCC_ARM uses the ModusToolbox
# IDE provided compiler by default).
CY_COMPILER_PATH=


# Locate ModusToolbox IDE helper tools folders in default installation
# locations for Windows, Linux, and macOS.
CY_WIN_HOME=$(subst \,/,$(USERPROFILE))
CY_TOOLS_PATHS ?= $(wildcard \
    $(CY_WIN_HOME)/ModusToolbox/tools_* \
    $(HOME)/ModusToolbox/tools_* \
    /Applications/ModusToolbox/tools_*)

# If you install ModusToolbox IDE in a custom location, add the path to its
# "tools_X.Y" folder (where X and Y are the version number of the tools
# folder). Make sure you use forward slashes.
CY_TOOLS_PATHS+=

# Default to the newest installed tools folder, or the users override (if it's
# found).
CY_TOOLS_DIR=$(lastword $(sort $(wildcard $(CY_TOOLS_PATHS))))

ifeq ($(CY_TOOLS_DIR),)
$(error Unable to find any of the available CY_TOOLS_PATHS -- $(CY_TOOLS_PATHS). On Windows, use forward slashes.)
endif

$(info Tools Directory: $(CY_TOOLS_DIR))

include $(CY_TOOLS_DIR)/make/start.mk
