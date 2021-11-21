/* This source file is part of the ATMEL AT32UC3A-SoftwareFramework-1.1.1 Release */

/*This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief External Interrupt Controller Example
 *
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with a Power Manager.
 * - AppNote:
 *
 * \author               Atmel Corporation: http://www.atmel.com \n
 *                       Support and FAQ: http://support.atmel.no/
 *
 *****************************************************************************/

/*! \page License
 * Copyright (c) 2007, Atmel Corporation All rights reserved.
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

/*! \mainpage
 * \section intro Introduction
 * This is the documentation for the data structures, functions, variables,
 * defines, enums, and typedefs for the EIC driver. This documentation is also
 * bundled with an example.
 *
 * This example shows how to use an external interrupt to wake up the
 * cpu when in sleep mode (static). \n
 * <b>Operating mode:</b>
 * -# At the beginning, CPU is in sleep mode.
 * -# The user needs to press the Joystick(on EVK1100) or PB0(on EVK1101) to
 * leave the sleep mode and the led LED0 toggles. After a short delay, CPU is
 * switched back into sleep mode (back to step 1)...etc
 *
 * \note on EVK1101 Rev. B, PB0 is named PB2.
 *
 * \section files Main Files
 * - eic.c: EIC driver;
 * - eic.h: EIC driver header file;
 * - eic_example1.c: EIC example application.
 *
 * \section compinfo Compilation Info
 * This software was written for the GNU GCC for AVR32 and IAR Systems compiler
 * for AVR32. Other compilers may or may not work.
 *
 * \section deviceinfo Device Info
 * All AVR32 devices with a EIC module can be used. This example has been tested
 * with the following setup:<BR>
 * <ul>
 *  <li>EVK1100 evaluation kit
 *  <li>EVK1101 evaluation kit
 *  </ul>
 *
 * \section setupinfo Setup Information
 * CPU speed: <i> Switch to oscillator external OSC0 = 12 Mhz. </i>
 *
 * \section contactinfo Contact Information
 * For further information, visit
 * <A href="http://www.atmel.com/products/AVR32/">Atmel AVR32</A>.\n
 * Support and FAQ: http://support.atmel.no/
 */

#include <avr32/io.h>
#include "compiler.h"
#include "gpio.h"
#include "pm.h"
#include "board.h"
#include "eic.h"

/*! \name External Interrupt pin Mappings
 */
//! @{
#if BOARD==EVK1100
// With the UC3A datasheet, in section "Peripheral Multiplexing on I/O lines" in the
// table "GPIO Controller Function Multiplexing", we see that the NMI is mapped
// on pin20 (i.e. PA20). Using the EVK1100 schematics, we see that the joystick
// push event is connected to PA20. Thus, a push on the joystick will generate a
// NMI. The External Interrupt number 8 is the NMI.
// Note: AVR32_EIC_EXTINT_8_PIN & AVR32_EIC_EXTINT_8_FUNCTION defines are found
// in the uc3axxxx.h part-specific header file.
#  define EXT_INT_EXAMPLE_PIN_LINE               AVR32_EIC_EXTINT_8_PIN
#  define EXT_INT_EXAMPLE_FUNCTION_LINE          AVR32_EIC_EXTINT_8_FUNCTION
#  define EXT_INT_EXAMPLE_LINE                   EXT_NMI
#elif BOARD==EVK1101
// With the UC3B datasheet, in section "Peripheral Multiplexing on I/O lines" in the
// table "GPIO Controller Function Multiplexing", we see that the External Interrupt 6
// is mapped on pin34 (i.e. PB02). Using the EVK1101 schematics, we see that the
// pushbutton 0 press event is connected to PB02. Thus, a press on PB0 will generate
// the external interrupt 6.
// \note On EVK1101 Rev. B, PB0 is named PB2.
// \note AVR32_EIC_EXTINT_6_PIN & AVR32_EIC_EXTINT_6_FUNCTION defines are found
// in the uc3bxxxx.h part-specific header file.
#  define EXT_INT_EXAMPLE_PIN_LINE               AVR32_EIC_EXTINT_6_PIN
#  define EXT_INT_EXAMPLE_FUNCTION_LINE          AVR32_EIC_EXTINT_6_FUNCTION
#  define EXT_INT_EXAMPLE_LINE                   EXT_INT6
#endif
//! @}


/* \brief Software Delay
 *
 */
static void software_delay(void)
{
  int i;
  for (i=0; i<10000; i++);
}

/*
 * \brief main function : do init and loop to wake up CPU through EIC controller
 */
int main(void)
{
  eic_options_t eic_options;  // Structure holding the configuration parameters
                              // of the EIC module.

  // Enable LED0_GPIO on the EVK
  gpio_enable_gpio_pin(LED0_GPIO);

  // Enable level-triggered interrupt.
  eic_options.eic_mode   = EIC_MODE_LEVEL_TRIGGERED;
  // Interrupt will trigger on low-level.
  eic_options.eic_level  = EIC_LEVEL_LOW_LEVEL;
  // Enable filter.
  eic_options.eic_filter  = EIC_FILTER_ENABLED;
  // For Wake Up mode, initialize in asynchronous mode
  eic_options.eic_async  = EIC_ASYNCH_MODE;
  // Choose External Interrupt Controller Line
  eic_options.eic_line = EXT_INT_EXAMPLE_LINE; // Enable the NMI external interrupt line.

  // Map the interrupt line to the GPIO pin with the right peripheral function.
  gpio_enable_module_pin(EXT_INT_EXAMPLE_PIN_LINE,EXT_INT_EXAMPLE_FUNCTION_LINE);

  // Init the EIC controller with the options
  eic_init(&AVR32_EIC, &eic_options,1);
  // Enable External Interrupt Controller Line
  eic_enable_line(&AVR32_EIC, EXT_INT_EXAMPLE_LINE);

  // Switch the CPU to static sleep mode.
  // When the CPU is idle, it is possible to switch off the CPU clock and optionally other
  // clock domains to save power. This is activated by the sleep instruction, which takes the sleep
  // mode index number as argument. SLEEP function is defined in \DRIVERS\PM\pm.h.
  // In static mode, all oscillators, including 32KHz and RC oscillator are stopped.
  // Bandgap voltage reference BOD detector is turned off.
  SLEEP(AVR32_PM_SMODE_STATIC);

  // Activate LED0 pin in GPIO output mode and switch LED0 off.
  gpio_set_gpio_pin(LED0_GPIO);

  // Cpu now is in static sleep mode. When the wake-up external interrupt occurs,
  // the CPU resumes execution here and enter the while(1) loop.
  while(1)
  {
    gpio_tgl_gpio_pin(LED0_GPIO); // Toggle the LED0.
    software_delay();             // Just a dummy finite loop.
    // Interrupt Line must be cleared to enable next SLEEP action
    eic_clear_interrupt_line(&AVR32_EIC, EXT_INT_EXAMPLE_LINE);

    SLEEP(AVR32_PM_SMODE_STATIC); // re-enter sleep mode.
    // Cpu now is in static sleep mode. When the wake-up external interrupt occurs,
    // the CPU resumes execution back from the top of the while loop.
  }

}
