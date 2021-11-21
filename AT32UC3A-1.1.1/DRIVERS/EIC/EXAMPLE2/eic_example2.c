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

/*! \mainpage
 * \section intro Introduction
 * This is the documentation for the data structures, functions, variables,
 * defines, enums, and typedefs for the EIC driver. This documentation is also
 * bundled with an example.
 *
 * This example shows how to use an external interrupt and to generate interrupt.\n
 * <b>Operating mode:</b>
 * - For EVK1100:\n
 *    - When the user presses JOYSTICK : the led LED0 switches ON for a short while(software delay) then it switches OFF\n
 * - For EVK1101:\n
 *    - When the user presses PB0 : the led LED0 switch ON for a short while(software delay) then it switches OFF / LED1 switches OFF\n
 *    - When the user presses PB1 : the led LED0 switch OFF / LED1 switches ON for a short while(software delay) then it switches OFF
 * \section files Main Files
 * - eic.c: EIC driver;
 * - eic.h: EIC driver header file;
 * - eic_example2.c: EIC example application.
 *
 * \section compinfo Compilation Info
 * This software was written for the GNU GCC for AVR32 and IAR Systems compiler
 * for AVR32. Other compilers may or may not work.
 *
 * \section deviceinfo Device Info
 * All AVR32 devices with a EIC module can be used. This example has been tested
 * with the following boards:
 * - EVK1100 evaluation kit
 * - EVK1101 evaluation kit
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
#include "intc.h"

/*! \name External Interrupt pin Mappings
 */
//! @{
// With the datasheet, in section "Peripheral Multiplexing on I/O lines" in the
// table "GPIO Controller Function Multiplexing",
// For UC3A part, linked to EVK1100 board, we see that PA20 is
// multiplexed with EXTINT lines NMI
// Note: PA20 is available with joystick push
#if BOARD==EVK1100
#define EXT_INT_EXAMPLE_PIN_LINE1               AVR32_EIC_EXTINT_8_PIN
#define EXT_INT_EXAMPLE_FUNCTION_LINE1          AVR32_EIC_EXTINT_8_FUNCTION
#define EXT_INT_EXAMPLE_LINE1                   EXT_NMI
#define EXT_INT_EXAMPLE_NB_LINES                1
#elif BOARD==EVK1101
// For UC3B part, linked to EVK1101 board, we see that PB2 and PB3 are
// multiplexed with EXTINT lines 6 and 7,
// Note: PB2, PB3 are available with buttons PB0 and PB1
#define EXT_INT_EXAMPLE_PIN_LINE1               AVR32_EIC_EXTINT_6_PIN
#define EXT_INT_EXAMPLE_FUNCTION_LINE1          AVR32_EIC_EXTINT_6_FUNCTION
#define EXT_INT_EXAMPLE_LINE1                   EXT_INT6
#define EXT_INT_EXAMPLE_IRQ_LINE1               AVR32_EIC_EIC_IRQ_6
#define EXT_INT_EXAMPLE_PIN_LINE2               AVR32_EIC_EXTINT_7_PIN
#define EXT_INT_EXAMPLE_FUNCTION_LINE2          AVR32_EIC_EXTINT_7_FUNCTION
#define EXT_INT_EXAMPLE_LINE2                   EXT_INT7
#define EXT_INT_EXAMPLE_IRQ_LINE2               AVR32_EIC_EIC_IRQ_7
#define EXT_INT_EXAMPLE_NB_LINES                2
#endif
//! @}


//! Structure holding the configuration parameters of the EIC module.
eic_options_t eic_options[EXT_INT_EXAMPLE_NB_LINES];

//! Global var used to switch between different LED patterns.
unsigned char key_number=0;

#if BOARD==EVK1100
/*!
 * \brief Interrupt handler of the External interrupt line "1" assigned to the NMI exception.
 *
 * \note This function is not static because it is referenced outside
 * (in exception.S for GCC and in exception.s82 for IAR).
 */
#if __GNUC__
__attribute__((__naked__))
#elif __ICCAVR32__
#pragma shadow_registers = full
#endif
void eic_nmi_handler( void )
{
  __asm__ __volatile__ (
    "pushm   r0-r12, lr\n\t"  /* Save registers not saved upon NMI exception. */
  );
  eic_clear_interrupt_line(&AVR32_EIC, EXT_INT_EXAMPLE_LINE1);
  key_number = 1;
  __asm__ __volatile__ (
    "popm   r0-r12, lr\n\t"   /* Restore the registers. */
    "rete"                    /* Leaving the exception handler. */
  );
}
#endif // #if BOARD==EVK1100

#if BOARD==EVK1101
/*!
 * \brief Interrupt handler of the External interrupt line "1".
 */
#if __GNUC__
__attribute__((__interrupt__))
#elif __ICCAVR32__
__interrupt
#endif
static void eic_int_handler1(void)
{
  eic_clear_interrupt_line(&AVR32_EIC, EXT_INT_EXAMPLE_LINE1);
  key_number = 1;
}

/*!
 * \brief Interrupt handler of the External interrupt line "2".
 */
#if __GNUC__
__attribute__((__interrupt__))
#elif __ICCAVR32__
__interrupt
#endif
static void eic_int_handler2(void)
{
  eic_clear_interrupt_line(&AVR32_EIC, EXT_INT_EXAMPLE_LINE2);
  key_number = 2;
}
#endif //if BOARD==EVK1101


/* \brief Software Delay
 *
 */
static void software_delay(void)
{
  int i;
  for (i=0; i<10000; i++);
}

/*
 * \brief main function : do init and wait interrupt catched by EIC controller.
 */
int main(void)
{
  // Enable edge-triggered interrupt.
  eic_options[0].eic_mode   = EIC_MODE_EDGE_TRIGGERED;
  // Interrupt will trigger on falling edge.
  eic_options[0].eic_edge  = EIC_EDGE_FALLING_EDGE;
  // Initialize in synchronous mode : interrupt is synchronized to the clock
  eic_options[0].eic_async  = EIC_SYNCH_MODE;
  // Set the interrupt line number.
  eic_options[0].eic_line   = EXT_INT_EXAMPLE_LINE1;

  // Only available for EVK1101
#if BOARD==EVK1101
  // Enable edge-triggered interrupt.
  eic_options[1].eic_mode   = EIC_MODE_EDGE_TRIGGERED;
  // Interrupt will trigger on falling edge.
  eic_options[1].eic_edge  = EIC_EDGE_FALLING_EDGE;
  // Initialize in synchronous mode : interrupt is synchronized to the clock
  eic_options[1].eic_async  = EIC_SYNCH_MODE;
  // Set the interrupt line number.
  eic_options[1].eic_line   = EXT_INT_EXAMPLE_LINE2;
#endif

  // Activate LED0 & LED1 pins in GPIO output mode and switch them off.
  gpio_set_gpio_pin(LED0_GPIO);
  gpio_set_gpio_pin(LED1_GPIO);

  // Map the interrupt lines to the GPIO pins with the right peripheral functions.
#if BOARD==EVK1100
  gpio_enable_module_pin(EXT_INT_EXAMPLE_PIN_LINE1,EXT_INT_EXAMPLE_FUNCTION_LINE1);
#elif BOARD==EVK1101
  static const gpio_map_t EIC_GPIO_MAP =
  {
    {EXT_INT_EXAMPLE_PIN_LINE1, EXT_INT_EXAMPLE_FUNCTION_LINE1},
    {EXT_INT_EXAMPLE_PIN_LINE2, EXT_INT_EXAMPLE_FUNCTION_LINE2}
  };
  gpio_enable_module(EIC_GPIO_MAP,sizeof(EIC_GPIO_MAP) / sizeof(EIC_GPIO_MAP[0]));
#endif

  // Disable all interrupts.
  Disable_global_interrupt();

#if BOARD==EVK1101
  // Initialize interrupt vectors.
  INTC_init_interrupts();

  // Register the USART interrupt handler to the interrupt controller.
  // usart_int_handler is the interrupt handler to register.
  // EXAMPLE_USART_IRQ is the IRQ of the interrupt handler to register.
  // INT0 is the interrupt priority level to assign to the group of this IRQ.
  // void INTC_register_interrupt(__int_handler handler, unsigned int irq, unsigned int int_lev);
  INTC_register_interrupt(&eic_int_handler1, EXT_INT_EXAMPLE_IRQ_LINE1, INT0);
  INTC_register_interrupt(&eic_int_handler2, EXT_INT_EXAMPLE_IRQ_LINE2, INT0);
#endif

  // On EVK1100, the JOYSTICK External interrupt line is assigned to the NMI.
  // Since the NMI is not an interrupt but an exception managed by the CPU, we have
  // to make sure that the NMI handler calls our handler: this is done in the
  // files exception.S(for GCC) & exception.s82(for IAR); look for the _handle_NMI
  // assembly label.

  // Init the EIC controller with the options
  eic_init(&AVR32_EIC, eic_options,EXT_INT_EXAMPLE_NB_LINES);

  // Enable the chosen lines and their corresponding interruption feature.
#if BOARD==EVK1100
  eic_enable_line(&AVR32_EIC, eic_options[0].eic_line);
  eic_enable_interrupt_line(&AVR32_EIC, eic_options[0].eic_line);
#elif BOARD==EVK1101
  eic_enable_lines(&AVR32_EIC, (1<<eic_options[1].eic_line)|(1<<eic_options[0].eic_line));
  eic_enable_interrupt_lines(&AVR32_EIC, (1<<eic_options[1].eic_line)|(1<<eic_options[0].eic_line));
#endif

  // Enable all interrupts.
  Enable_global_interrupt();

  // Loop infinitely, toggling the leds depending on the latest external event.
  while(1)
  {
    switch (key_number)
    {
      case 1 :
        gpio_clr_gpio_pin(LED0_GPIO);
        gpio_set_gpio_pin(LED1_GPIO);
        key_number = 0;
        software_delay();
        break;
      case 2 :
        gpio_set_gpio_pin(LED0_GPIO);
        gpio_clr_gpio_pin(LED1_GPIO);
        key_number = 0;
        software_delay();
        break;
      default :
        gpio_set_gpio_pin(LED0_GPIO);
        gpio_set_gpio_pin(LED1_GPIO);
        break;
    }
  }

}
