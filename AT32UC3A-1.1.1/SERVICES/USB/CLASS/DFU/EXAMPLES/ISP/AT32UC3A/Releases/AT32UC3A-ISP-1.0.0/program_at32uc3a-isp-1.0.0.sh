#!/bin/sh

# This shell script programs the released ISP (flash array), the ISP
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
echo Performing a JTAG Chip Erase command.
avr32program chiperase
sleep 9

echo
echo Programming MCU memory from \`at32uc3a-isp-1.0.0.hex\'.
avr32-objcopy -I ihex -O binary at32uc3a-isp-1.0.0.hex at32uc3a-isp-1.0.0.bin
avr32program program -finternal@0x80000000,512Kb -cxtal -e -v -O0x80000000 -Fbin at32uc3a-isp-1.0.0.bin
echo
rm -f at32uc3a-isp-1.0.0.bin
sleep 9

echo
echo Programming ISP configuration word.
echo -ne "\x92\x9E\x14\x24" > at32uc3a-isp_cfg-1.0.0.bin
avr32program program -finternal@0x80000000,512Kb -cxtal -e -v -O0x808001FC -Fbin at32uc3a-isp_cfg-1.0.0.bin
echo
rm -f at32uc3a-isp_cfg-1.0.0.bin
sleep 9

echo
echo Programming general-purpose fuse bits.
avr32program writefuses -finternal@0x80000000,512Kb gp=0xFC07FFFF
sleep 9

echo
echo Starting CPU execution.
avr32program run -R
echo
