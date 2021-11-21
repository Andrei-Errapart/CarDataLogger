# Hey Emacs, this is a -*- makefile -*-

# The purpose of this file is to define the build configuration variables used
# by the generic Makefile. See Makefile header for further information.

# Copyright (c) 2007, Atmel Corporation All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation and/
# or other materials provided with the distribution.
#
# 3. The name of ATMEL may not be used to endorse or promote products derived
# from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY ATMEL ``AS IS'' AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY AND
# SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
# EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


# Base paths
PRJ_PATH = ../AT32UC3A-1.1.1
APPS_PATH = $(PRJ_PATH)/APPLICATIONS
BRDS_PATH = $(PRJ_PATH)/BOARDS
COMP_PATH = $(PRJ_PATH)/COMPONENTS
DRVR_PATH = $(PRJ_PATH)/DRIVERS
SERV_PATH = $(PRJ_PATH)/SERVICES
UTIL_PATH = $(PRJ_PATH)/UTILS

# CPU architecture: {ap|uc}
ARCH = uc

# Part: {none|ap7xxx|uc3xxxxx}
PART = uc3a0512

# Flash memories: [{cfi|internal}@address,size]...
FLASH = internal@0x80000000,512Kb

# Clock source to use when programming: [{xtal|extclk|int}]
PROG_CLOCK = xtal

# Device/Platform/Board include path
PLATFORM_INC_PATH = \
  $(BRDS_PATH)/

# Target name: {*.a|*.elf}
TARGET = datalogger.elf

# Definitions: [-D name[=definition]...] [-U name...]
# Things that might be added to DEFS:
#   BOARD             Board used: {EVKxxxx}
#   EXT_BOARD         Extension board used (if any): {EXTxxxx}
DEFS = -D BOARD=EVK1100 #-DFILESYSTEM_DEBUG #-DTRACE_SENSORS_TIMING #-D _ASSERT_ENABLE_
#DEFS = -D BOARD=EVK1100 -DTRACE_SENSORS_TIMING #-DFILESYSTEM_DEBUG #-D _ASSERT_ENABLE_

# Include path
INC_PATH = \
  $(UTIL_PATH)/ \
  $(UTIL_PATH)/PREPROCESSOR/ \
  $(UTIL_PATH)/DEBUG/ \
  $(BRDS_PATH)/EVK1100 \
  $(COMP_PATH)/DISPLAY/DIP204/ \
  $(COMP_PATH)/MEMORY/SDRAM \
  $(SERV_PATH)/USB/CLASS/DFU/EXAMPLES/ISP/BOOT/ \
  $(DRVR_PATH)/CPU/CYCLE_COUNTER/ \
  $(DRVR_PATH)/INTC/ \
  $(DRVR_PATH)/USART/ \
  $(DRVR_PATH)/GPIO/ \
  $(DRVR_PATH)/SPI/ \
  $(DRVR_PATH)/PWM/ \
  $(DRVR_PATH)/PM/ \
  $(DRVR_PATH)/TC/ \
  $(DRVR_PATH)/FLASHC \
  $(DRVR_PATH)/EBI/SDRAMC \
  ../Filesystem \
  .

# C source files
CSRCS = \
  $(BRDS_PATH)/EVK1100/led.c \
  $(DRVR_PATH)/USART/usart.c \
  $(DRVR_PATH)/INTC/intc.c \
  $(DRVR_PATH)/GPIO/gpio.c \
  $(DRVR_PATH)/SPI/spi.c \
  $(DRVR_PATH)/PWM/pwm.c \
  $(DRVR_PATH)/PM/pm.c \
  $(DRVR_PATH)/TC/tc.c \
  $(DRVR_PATH)/FLASHC/flashc.c \
  $(DRVR_PATH)/EBI/SDRAMC/sdramc.c \
  $(COMP_PATH)/DISPLAY/DIP204/dip204.c \
  Utils.c IClock.c ITimer.c IUsart.c \
  tprintf.c 

CXXSRCS := \
  Gps.cpp AccelerationSensors.cpp			\
  Display.cpp						\
  main.cpp						\
  LoggerConfig.cpp 					\
  ../Filesystem/Filesystem/Blockdevice.cpp		\
  ../Filesystem/Filesystem/Blockdevice_File.cpp		\
  ../Filesystem/Filesystem/Blockdevice_SDMMC.cpp	\
  ../Filesystem/Filesystem/Endian.cpp			\
  ../Filesystem/Filesystem/Error.cpp			\
  ../Filesystem/Filesystem/FAT16.cpp			\
  ../Filesystem/Filesystem/File.cpp			\
  ../Filesystem/Filesystem/Config.cpp


# Assembler source files
ASSRCS = \
  $(SERV_PATH)/USB/CLASS/DFU/EXAMPLES/ISP/BOOT/trampoline.S \
  $(DRVR_PATH)/INTC/exception.S

# Library path
LIB_PATH =

# Libraries to link with the project
LIBS =

# Linker script file if any
LINKER_SCRIPT = $(UTIL_PATH)/LINKER_SCRIPTS/AT32UC3A/0512/GCC/link_uc3a0512.lds

# Options to request or suppress warnings: [-fsyntax-only] [-pedantic[-errors]] [-w] [-Wwarning...]
# For further details, refer to the chapter "GCC Command Options" of the GCC manual.
WARNINGS = -Wall

# Options for debugging: [-g]...
# For further details, refer to the chapter "GCC Command Options" of the GCC manual.
DEBUG = -g

# Options that control optimization: [-O[0|1|2|3|s]]...
# For further details, refer to the chapter "GCC Command Options" of the GCC manual.
OPTIMIZATION = -O2 -ffunction-sections -fdata-sections

# Extra flags to use when preprocessing
CPP_EXTRA_FLAGS =

# Extra flags to use when compiling
C_EXTRA_FLAGS =

# Extra flags to use when assembling
AS_EXTRA_FLAGS =

# Extra flags to use when linking
LD_EXTRA_FLAGS = -Wl,--gc-sections -Wl,-e,_trampoline
#LD_EXTRA_FLAGS = -Wl,--gc-sections -mrelax -Wl,-e,_trampoline

# Documentation path
DOC_PATH = \
  ../../DOC/

# Documentation configuration file
DOC_CFG = \
  ../doxyfile.doxygen
