#!/bin/sh

# Usage: isp_cfg.sh <isp_cfg_io_cond_pin> <isp_cfg_io_cond_level> [<binfile>]
#
# This shell script computes the ISP configuration word from the arguments.
#
# <isp_cfg_io_cond_pin>     ISP I/O condition GPIO pin. Possible values are 0 to
#                           the number of the last GPIO pin.
#
# <isp_cfg_io_cond_level>   ISP I/O condition active level. Possible values are
#                           0 and 1.
#
# [<binfile>]               If specified, the configuration word is written to
#                           this binary file, else it is output to stdout.

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


# This function counts the leading zeros of the 32-bit input value.
clz()
{
  ((u32 = $1))
  for ((i = 31; i >= 0 && !(u32 & 1 << i); i--)); do :; done
  echo $((31 - i))
}


# This function computes the 8-bit CRC of the 32-bit input value passed as
# second argument using the CRC polynomial passed as first argument.
crc8()
{
  ((crc8_polynomial = $1))
  ((_crc8 = $2))
  while ((_crc8 > 0xFF))
  do
    ((_crc8 ^= crc8_polynomial << (32 - `clz _crc8` - 9)))
  done
  echo $((_crc8))
}


# Constant definitions.
((ISP_CFG_CRC8_POLYNOMIAL = 0x107))
((ISP_CFG_BOOT_KEY = 0x494F))

# Assign arguments to variables with explicit names.
((isp_cfg_io_cond_pin = $1))
((isp_cfg_io_cond_level = $2))

# Compute the ISP configuration word.
((isp_cfg = isp_cfg_io_cond_pin << 8 | isp_cfg_io_cond_level << 16 | ISP_CFG_BOOT_KEY << 17))
((isp_cfg |= `crc8 ISP_CFG_CRC8_POLYNOMIAL isp_cfg`))

# If [<binfile>] is specified,
if (($# >= 3))
then
  # write the ISP configuration word to this binary file,
  printf %.8X $isp_cfg | sed 's/[0-9A-F]\{2\}/\\\\x\0/g' | xargs echo -ne > $3
# else
else
  # output it to stdout.
  printf "ISP configuration word: 0x%.8X\n" $isp_cfg
fi
