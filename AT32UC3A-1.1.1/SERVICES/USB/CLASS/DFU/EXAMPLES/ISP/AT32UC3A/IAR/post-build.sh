#!/bin/sh

# This shell script creates a binary image and an IAR EWAVR32 assembly header
# file from the Intel HEX image and then programs the ISP (flash array), the ISP
# configuration word (User page) and the general-purpose fuse bits.

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


echo
echo Creating \`at32uc3a-isp.bin\' from \`at32uc3a-isp.hex\'.
avr32-objcopy -I ihex -O binary at32uc3a-isp.hex at32uc3a-isp.bin

echo
echo Creating \`at32uc3a-isp.h\' from \`at32uc3a-isp.bin\'.
od -A n -t x1 at32uc3a-isp.bin | sed -e 's/[0-9a-fA-F]\+/0x\U\0,/g' -e 's/\(.*\),/  DC8\1\r/' > at32uc3a-isp.h

echo
echo Performing a JTAG Chip Erase command.
avr32program chiperase
sleep 3

echo
echo Programming MCU memory from \`at32uc3a-isp.bin\'.
avr32program program -finternal@0x80000000,512Kb -cxtal -e -v -O0x80000000 -Fbin at32uc3a-isp.bin
echo
sleep 3

echo
echo Programming ISP configuration word.
../../isp_cfg.sh 20 0 at32uc3a-isp_cfg.bin
avr32program program -finternal@0x80000000,512Kb -cxtal -e -v -O0x808001FC -Fbin at32uc3a-isp_cfg.bin
echo
rm -f at32uc3a-isp_cfg.bin
sleep 3

echo
echo Programming general-purpose fuse bits.
avr32program writefuses -finternal@0x80000000,512Kb gp=0xFC07FFFF
sleep 3

echo
echo Resetting MCU.
avr32program reset
echo
