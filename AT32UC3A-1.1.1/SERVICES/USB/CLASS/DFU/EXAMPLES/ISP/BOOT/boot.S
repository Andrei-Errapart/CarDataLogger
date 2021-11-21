/* This file is part of the ATMEL AT32UC3A-SoftwareFramework-1.1.1 Release */

/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief AVR32 UC3 ISP boot.
 *
 * - Compiler:           GNU GCC for AVR32
 * - Supported devices:  All AVR32UC devices with an INTC module can be used.
 *
 * \author               Atmel Corporation: http://www.atmel.com \n
 *                       Support and FAQ: http://support.atmel.no/
 *
 ******************************************************************************/

/* Copyright (c) 2007, Atmel Corporation All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of ATMEL may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY AND
 * SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <avr32/io.h>
#include "conf_isp.h"


//! @{
//! \verbatim


  // Performs efficiently a bitwise logical Exclusive-OR between the specified
  // register and an immediate value of up to 32 bits. The result is stored in
  // the destination register.
  .macro  eor.w   rd, imm
    .if \imm & 0x0000FFFF
      eorl    \rd, LO(\imm)
    .endif
    .if \imm & 0xFFFF0000
      eorh    \rd, HI(\imm)
    .endif
  .endm

  // Moves efficiently an immediate value of up to 32 bits into a register.
  .macro  mov.w   rd, imm
    .if \imm & 0xFFF00000
      mov     \rd, LO(\imm)
      orh     \rd, HI(\imm)
    .else
      mov     \rd, \imm
    .endif
  .endm

  // Performs efficiently a bitwise logical OR between the specified register
  // and an immediate value of up to 32 bits. The result is stored in the
  // destination register.
  .macro  or.w    rd, imm
    .if \imm & 0x0000FFFF
      orl     \rd, LO(\imm)
    .endif
    .if \imm & 0xFFFF0000
      orh     \rd, HI(\imm)
    .endif
  .endm


  .section  .reset, "ax", @progbits


  .balign 2

  // Reset vector: This must be linked @ 0x80000000.
  .global _start
  .type _start, @function
_start:

  // Start of exception vector table: Unrecoverable exception.
  .global _evba
  .type _evba, @function
_evba:
  mfsr    r8, AVR32_SR
  bfextu  r8, r8, AVR32_SR_M0_OFFSET, AVR32_SR_M0_SIZE + AVR32_SR_M1_SIZE + AVR32_SR_M2_SIZE
  cp.w    r8, 0b001
  breq    boot_supervisor_mode
  sub     r8, pc, $ - boot_supervisor_mode
  mov.w   r9, AVR32_SR_GM_MASK | AVR32_SR_EM_MASK | AVR32_SR_M0_MASK
  mov     sp, _estack - 6 * 4
  pushm   r8-r9
  rete

boot_supervisor_mode:
  mov     r8, ISP_KEY_ADDRESS
  mov     r9, AVR32_WDT_ADDRESS
  mov     r10, AVR32_FLASHC_ADDRESS
  mov     r11, AVR32_PM_ADDRESS
  ld.w    r0, r8[0]
  mov.w   r1, ISP_KEY_VALUE
  ld.w    r2, r10[AVR32_FLASHC_FGPFR]
  bld     r2, ISP_GPFB_BOD_EN_OFFSET
  brcc    test_isp_gpfb_force
  ld.w    r3, r11[AVR32_PM_BOD]
  mov.w   r4, AVR32_BOD_KEY << AVR32_PM_BOD_KEY_OFFSET
  or.w    r3, AVR32_PM_BOD_FCD_MASK |\
              ((~AVR32_BOD_KEY << AVR32_PM_BOD_KEY_OFFSET) & AVR32_PM_BOD_KEY_MASK)
  mov     r5, AVR32_PM_BOD_CTRL_ENABLED_NORESET
  bfins   r3, r5, AVR32_PM_BOD_CTRL_OFFSET, AVR32_PM_BOD_CTRL_SIZE
  st.w    r11[AVR32_PM_BOD], r4
  st.w    r11[AVR32_PM_BOD], r3
  mov     r5, AVR32_PM_BOD_CTRL_ENABLED
  bfins   r3, r5, AVR32_PM_BOD_CTRL_OFFSET, AVR32_PM_BOD_CTRL_SIZE
  st.w    r11[AVR32_PM_BOD], r4
  st.w    r11[AVR32_PM_BOD], r3
test_isp_gpfb_force:
  bld     r2, ISP_GPFB_FORCE_OFFSET
  brcs    start_loader
  ld.w    r3, r11[AVR32_PM_RCAUSE]
  mov.w   r4, AVR32_PM_RCAUSE_POR_MASK |\
              AVR32_PM_RCAUSE_EXT_MASK |\
              AVR32_PM_RCAUSE_JTAG_MASK |\
              AVR32_PM_RCAUSE_OCDRST_MASK |\
              AVR32_PM_RCAUSE_JTAGHARD_MASK
  tst     r3, r4
  brne    manage_io_cond
  bld     r3, AVR32_PM_RCAUSE_WDT_OFFSET
  brcs    start_program
  cp.w    r0, r1
  brne    start_program_no_isp_key

start_loader:
  rcall   disable_wdt
  st.w    r8[0], r1

  // Set initial stack pointer.
  mov     sp, _estack

  // Disable the exception processing.
  ssrf    AVR32_SR_EM_OFFSET

  // Set up EVBA so interrupts can be enabled.
  sub     r0, pc, $ - _evba
  mtsr    AVR32_EVBA, r0

  // Load initialized data having a global lifetime from the data LMA.
  mov     r0, _data
  mov     r1, _edata
  sub     r2, pc, $ - _data_lma
  rjmp    test_load_idata_end
load_idata:
  ld.d    r4, r2++
  st.d    r0++, r4
test_load_idata_end:
  cp.w    r0, r1
  brlo    load_idata

  // Clear uninitialized data having a global lifetime in the blank static storage section.
  mov     r0, __bss_start
  mov     r1, _end
  mov     r2, 0
  mov     r3, 0
  rjmp    test_clear_udata_end
clear_udata:
  st.d    r0++, r2
test_clear_udata_end:
  cp.w    r0, r1
  brlo    clear_udata

  // Call the ISP main function, which must not return.
  rcall   main

manage_io_cond:
  bld     r2, ISP_GPFB_IO_COND_EN_OFFSET
  brcc    start_program
  mov.w   r10, ISP_CFG_ADDRESS
  ld.w    r2, r10[0]
  mov     r3, ISP_CFG_CRC8_POLYNOMIAL
  mov     r4, r2
  rjmp    test_crc8_end
crc8:
  clz     r5, r4
  rsub    r5, r5, 32 - 9
  lsl     r5, r3, r5
  eor     r4, r5
test_crc8_end:
  cp.w    r4, 0xFF
  brhi    crc8
  cp.w    r4, 0
  brne    start_loader
  bfextu  r3, r2, ISP_CFG_BOOT_KEY_OFFSET, ISP_CFG_BOOT_KEY_SIZE
  cp.w    r3, ISP_CFG_BOOT_KEY_VALUE
  brne    start_loader
  bfextu  r3, r2, ISP_CFG_IO_COND_PIN_OFFSET, ISP_CFG_IO_COND_PIN_SIZE
  cp.w    r3, AVR32_GPIO_NUMBER_OF_PINS
  brhs    start_loader
  mov     r10, AVR32_GPIO_ADDRESS
  lsr     r4, r3, 5
  lsl     r4, 8
  add     r10, r4
  ld.w    r4, r10[AVR32_GPIO_PVR0]
  andl    r3, 0x1F
  lsr     r4, r4, r3
  bfextu  r3, r2, ISP_CFG_IO_COND_LEVEL_OFFSET, ISP_CFG_IO_COND_LEVEL_SIZE
  eor     r4, r3
  bld     r4, 0
  brcc    start_loader

start_program:
  cp.w    r0, r1
  brne    start_program_no_isp_key
  rcall   disable_wdt
  .global boot_program
  .type boot_program, @function
boot_program:
  mov     r8, ISP_KEY_ADDRESS
  mov     r0, 0
  st.w    r8[0], r0
start_program_no_isp_key:
  mov.w   r0, AVR32_SR_GM_MASK | AVR32_SR_EM_MASK | AVR32_SR_M0_MASK
  mtsr    AVR32_SR, r0
  .irp    rd, r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, sp, lr
    mov     \rd, 0
  .endr
  mtsr    AVR32_EVBA, r0
  mtsr    AVR32_COUNT, r0
  lddpc   pc, program_start_address

disable_wdt:
  mov.w   r2, AVR32_WDT_KEY_VALUE << AVR32_WDT_CTRL_KEY_OFFSET
  st.w    r9[AVR32_WDT_CTRL], r2
  eor.w   r2, AVR32_WDT_CTRL_KEY_MASK
  st.w    r9[AVR32_WDT_CTRL], r2
  mov     pc, lr


// Constant data area.

  .balign 4

program_start_address:
  .word PROGRAM_START_ADDRESS


//! \endverbatim
//! @}
