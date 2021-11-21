/* This source file is part of the ATMEL AT32UC3A-SoftwareFramework-1.1.1 Release */

/*This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief COUNT & COMPARE usage example.
 *
 * Example of COUNT & COMPARE registers usage, while using the USART SW module
 * (for printing ASCII msgs), the GPIO SW module (to map the USART on I/O pins),
 * the INTC SW module (for interrupt management).
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices.
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
 *
 * This documents gives an example of the usage of the CPU Cycle counter. The cycle counter is a COUNT register,
 * that increments once every clock cycle, regardless of pipeline stalls an
 * flushes. The COUNT register can both be read and written. The count register can be use
 * together with the COMPARE register to create a timer with interrupt functionality. The COUNT
 * register is written to zero upon reset. Incrementation of the COUNT register can not be disabled
 * The COUNT register will increment even though a compare interrupt is pending.
 *
 * \section example-description Example's Operating Mode
 * This example shows how to use the COUNT register together with the COMPARE register
 * to generate an interrupt periodically. Here is the operating mode of the example:
 * - At the beginning of the code, we check that initial default values of the COUNT
 * and COMPARE registers are correct.
 * - Then, the COUNT & COMPARE interrupt mechainsm is tested with a short delay. Messages
 * are displayed on USART1.
 * - Then the program infinitly loops, using the COUNT & COMPARE interrupt mechanism
 * with a longer delay. Messages are displayed on USART1 and one of Led 1 through Led4
 * is on upon each COUNT & COMPARE match (Led1 -> Led2 -> Led3 -> Led4 -> Led1 ...and so on).
 *
  \section files Main Files
 * - cycle_count_example.c : cycle counter example
 * - count.h: cycle counter driver interface
 *
 * \section compinfo Compilation Info
 * This software was written for the GNU GCC for AVR32 and IAR Systems compiler
 * for AVR32. Other compilers may or may not work.
 *
 * \section deviceinfo Device Info
 * All AVR32 devices. This example has been tested with the following setup:<BR>
 * <ul>
 *  <li>EVK1100 evaluation kit
 *  <li>EVK1101 evaluation kit
 *  </ul>
 *
 * \section setupinfo Setup Information
 * CPU speed: <i> Switch to oscillator external OSC0 = 12 Mhz. </i>
 *
 * \section configinfo Configuration Information
 * This example has been tested with the following configuration:
 * - USART1 connected to a PC serial port via a standard RS232 DB9 cable;
 * - PC terminal settings:
 *   - 57600 bps,
 *   - 8 data bits,
 *   - no parity bit,
 *   - 1 stop bit,
 *   - no flow control.
 *
 * \section contactinfo Contact Information
 * For further information, visit
 * <A href="http://www.atmel.com/products/AVR32/">Atmel AVR32</A>.\n
 * Support and FAQ: http://support.atmel.no/
 */


#if __GNUC__
#  include "intc.h"
#endif

#define _ASSERT_ENABLE_
#include "compiler.h"
#include "print_funcs.h"
#include "board.h"
#include "count.h"
#include "pm.h"


#define NB_CLOCK_CYCLE_DELAY_SHORT    1000000
#define NB_CLOCK_CYCLE_DELAY_LONG    20000000


static volatile unsigned int u32NbCompareIrqTrigger = 0;
static volatile unsigned char u8DisplayMsg = 0;

#if __GNUC__
__attribute__((__interrupt__))
#elif __ICCAVR32__
#pragma handler = AVR32_CORE_IRQ_GROUP, 3
__interrupt
#endif
static void compare_irq_handler(void)
{
  // Count the number of times this IRQ handler is called.
  u32NbCompareIrqTrigger++;
  u8DisplayMsg = 1; // Inform the main program that it may display a msg saying
                    // that the COUNT&COMPARE interrupt occurred.
                    // Disable the compare and exception generation feature: set COMPARE to 0.
  Set_sys_compare(0);
}

/* Main function */
int main(void)
{
   U32 u32CompareVal;
   U32 u32ComparePreviousVal = 0;
   U32 u32CompareValVerif;
   U32 u32CountVal;
   U32 u32CountNextVal;
   U8  u8LedMap = 0x01;
   volatile avr32_pm_t* pm = &AVR32_PM;

   // Switch the main clock to OSC0
   pm_switch_to_osc0(pm, FOSC0, OSC0_STARTUP);
   // Init DEBUG module
   init_dbg_rs232(FOSC0);

   print_dbg("---------------------------------------------\n");

   // Read COMPARE register.
   // NOTE: it should be equal to 0 (default value upon reset) => The compare
   // and exception generation feature is thus currently disabled.
   u32CompareVal = Get_sys_compare();
   Assert(!u32CompareVal);

   // Read COUNT register.
   // NOTE: the COUNT register increments since reset => it should be != 0.
   u32CountVal = Get_sys_count();
   Assert(u32CountVal);

#if __GNUC__
   // Disable all interrupts.
   Disable_global_interrupt();

   INTC_init_interrupts();

   // Register the compare interrupt handler to the interrupt controller.
   // compare_irq_handler is the interrupt handler to register.
   // AVR32_CORE_COMPARE_IRQ is the IRQ of the interrupt handler to register.
   // INT0 is the interrupt priority level to assign to the group of this IRQ.
   // void INTC_register_interrupt(__int_handler handler, unsigned int irq, unsigned int int_lev);
   INTC_register_interrupt(&compare_irq_handler, AVR32_CORE_COMPARE_IRQ, INT0);
#endif
   // Enable all interrupts.
   Enable_global_interrupt();

   // Schedule the COUNT&COMPARE match interrupt in NB_CLOCK_CYCLE_DELAY_SHORT 
   // clock cycles from now.
   u32CountVal = Get_sys_count();

   u32CompareVal = u32CountVal + NB_CLOCK_CYCLE_DELAY_SHORT; // WARNING: MUST FIT IN 32bits.
   // If u32CompareVal ends up to be 0, make it 1 so that the COMPARE and exception
   // generation feature does not get disabled.
   if(0 == u32CompareVal)
   {
      u32CompareVal++;
   }

   Set_sys_compare(u32CompareVal); // GO

   // Check if the previous write in the COMPARE register succeeded.
   u32CompareValVerif = Get_sys_compare();
   Assert( u32CompareVal==u32CompareValVerif );

   //  The previous COMPARE write succeeded.
   // Loop until the COUNT&COMPARE match triggers.
   while (!u32NbCompareIrqTrigger)
   {
      u32CountNextVal = Get_sys_count();

      if (u32CountNextVal < u32CompareVal)
         print_dbg("COUNT HAS NOT REACHED COMPARE YET (INFO)\n");
      else if (u32CountNextVal > u32CompareVal)
         print_dbg("COUNT IS GREATER THAN COMPARE (INFO)\n");
      else
         print_dbg("COUNT IS EQUAL TO COMPARE (INFO)\n");
   }

   while (TRUE)
   {
      if (u8DisplayMsg)
      {
         u8DisplayMsg = 0; // Reset

         // Turn the current LED on only and move to next LED.
         LED_Display_Field(LED_MONO0_GREEN |
                           LED_MONO1_GREEN |
                           LED_MONO2_GREEN |
                           LED_MONO3_GREEN,
                           u8LedMap);
         u8LedMap = max((U8)(u8LedMap << 1) & 0x0F, 0x01);

         // Print some info on the debug port.
         print_dbg("\nCOMPARE INTERRUPT TRIGGERED (OK): #");
         print_dbg_ulong(u32NbCompareIrqTrigger);

         // Schedule the COUNT&COMPARE match interrupt in NB_CLOCK_CYCLE_DELAY_LONG
         // clock cycles from now.
         u32CountVal = Get_sys_count();

         u32CompareVal = u32CountVal + NB_CLOCK_CYCLE_DELAY_LONG;
         // If u32CompareVal ends up to be 0, make it 1 so that the COMPARE and
         // exception generation feature does not get disabled.
         if(0 == u32CompareVal)
         {
            u32CompareVal++;
         }

         Set_sys_compare(u32CompareVal); // GO

         print_dbg("\n\tLatest COUNT == ");
         print_dbg_hex(u32CountVal);
         print_dbg(", COMPARE == ");
         print_dbg_hex(u32CompareVal);

         // What happens when COUNT reaches the 32-bit limit:
         // registers nicely loop around 0.
         if (u32CompareVal < u32ComparePreviousVal)
         {
            print_dbg("\nHere we are! Breaking the 32-bit limit!\n");
         }
         u32ComparePreviousVal = u32CompareVal;
      }
   }
}
