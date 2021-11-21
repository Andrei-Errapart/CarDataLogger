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
 * This example shows how to use the SCAN KEYPAD interface. It is not straightforward
 * to access this feature on the EVKs. Thus, an external connection should be done
 * using wires between EXTINT lines and SCAN lines.
 * <b>Operating mode:</b> In order to get key_number decoded, user should establish
 * manual connection(using the extension connectors zone) between SCAN lines and
 * EXTINT lines, as indicated in the arrays here below. As long as a key is "pressed",
 * its associated LED is on.
 * 
 * <table>
 * <CAPTION><em>Rows & columns view on the EVK1100</em></CAPTION>
 * <tr>
 *  <td>  </td>
 *  <td> SCAN 1(PA25) </td>
 *  <td> SCAN 2(PA26) </td>
 * </tr>
 * <tr>
 *  <td> EXTINT 1(PA10) </td>
 *  <td> Key 1 -> LED1 </td>
 *  <td> Key 2 -> LED2</td>
 * </tr>
 * <tr>
 *  <td> EXTINT 2(PA08) </td>
 *  <td> Key 3 -> LED3 </td>
 *  <td> Key 4 -> LED4 </td>
 * </tr>
 * </table>
 *
 * <table>
 * <CAPTION><em>Rows & columns view on the EVK1101</em></CAPTION>
 * <tr>
 *  <td>  </td>
 *  <td> SCAN 1(PB06) </td>
 *  <td> SCAN 2(PB07) </td>
 * </tr>
 * <tr>
 *  <td> EXTINT 1(PB02) </td>
 *  <td> Key 1 -> LED1 </td>
 *  <td> Key 2 -> LED2</td>
 * </tr>
 * <tr>
 *  <td> EXTINT 2(PB03) </td>
 *  <td> Key 3 -> LED3 </td>
 *  <td> Key 4 -> LED4 </td>
 * </tr>
 * </table>
 *
 *
 * \section files Main Files
 * - eic.c: EIC driver;
 * - eic.h: EIC driver header file;
 * - eic_example3.c: EIC example application.
 *
 * \section compinfo Compilation Info
 * This software was written for the GNU GCC for AVR32 and IAR Systems compiler
 * for AVR32. Other compilers may or may not work.
 *
 * \section deviceinfo Device Info
 * All AVR32 devices with a EIC module can be used. This example has been tested
 * on the following boards:<BR>
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


/*! \name Keypad scan index configuration
 * Choose the scan index for each scan column in the [0,7] range.\n
 * \warning: caution should be taken in the choice of this index regarding the
 * board schematics. For instance, on EVK1100, if scan[4] & scan[5] were chosen,
 * LED5 will blink at the keypad scan rate because LED5 and scan[4] and scan[5]
 * are using the same pins(PB19 & PB20).
 */
//! @{
#if BOARD==EVK1100
#define SCAN_COL1_IDX   0   //! Use the keypad scan[0] for column 1, multiplexed on PA25.
#define SCAN_COL2_IDX   1   //! Use the keypad scan[1] for column 2, multiplexed on PA26.
#elif BOARD==EVK1101
#define SCAN_COL1_IDX   4   //! Use the keypad scan[4] for column 1, multiplexed on PB06.
#define SCAN_COL2_IDX   5   //! Use the keypad scan[5] for column 2, multiplexed on PB07.
#endif
//! @}


/*! \name External Interrupt pin Mappings
 */
//! @{
// With the datasheet, in section "Peripheral Multiplexing on I/O lines" in the
// table "GPIO Controller Function Multiplexing", 
// For UC3A part, linked to EVK1100 board, we see that PA08 and PA10 are
// multiplexed with respectively EXTINT lines 7 and 6. PA25 and PA26 are respectively
// multiplexed with respectively SCAN lines 1 and 2.
// Note: PA08, PA10, PA25 and PA26 are available on extension connectors
#if BOARD==EVK1100
#define EXT_INT_EXAMPLE_PIN_LINE1               AVR32_EIC_EXTINT_6_PIN
#define EXT_INT_EXAMPLE_FUNCTION_LINE1          AVR32_EIC_EXTINT_6_FUNCTION
#define EXT_INT_EXAMPLE_LINE1                   EXT_INT6
#define EXT_INT_EXAMPLE_IRQ_LINE1               AVR32_EIC_EIC_IRQ_6
#define EXT_INT_EXAMPLE_PIN_LINE2               AVR32_EIC_EXTINT_7_PIN
#define EXT_INT_EXAMPLE_FUNCTION_LINE2          AVR32_EIC_EXTINT_7_FUNCTION
#define EXT_INT_EXAMPLE_LINE2                   EXT_INT7
#define EXT_INT_EXAMPLE_IRQ_LINE2               AVR32_EIC_EIC_IRQ_7
// For UC3B part, linked to EVK1101 board, we see that PB2 and PB3 are 
// multiplexed with EXTINT lines 6 and 7, and PB6 and PB7 with SCAN lines 4 and 5
// Note: PB2, PB3, PB6 and PB7 are available on extension connectors
#elif BOARD==EVK1101
#define EXT_INT_EXAMPLE_PIN_LINE1               AVR32_EIC_EXTINT_6_PIN
#define EXT_INT_EXAMPLE_FUNCTION_LINE1          AVR32_EIC_EXTINT_6_FUNCTION
#define EXT_INT_EXAMPLE_LINE1                   EXT_INT6
#define EXT_INT_EXAMPLE_IRQ_LINE1               AVR32_EIC_EIC_IRQ_6
#define EXT_INT_EXAMPLE_PIN_LINE2               AVR32_EIC_EXTINT_7_PIN
#define EXT_INT_EXAMPLE_FUNCTION_LINE2          AVR32_EIC_EXTINT_7_FUNCTION
#define EXT_INT_EXAMPLE_LINE2                   EXT_INT7
#define EXT_INT_EXAMPLE_IRQ_LINE2               AVR32_EIC_EIC_IRQ_7
#endif
#define EXT_SCAN_EXAMPLE_PIN_LINE1              ATPASTE3(AVR32_EIC_SCAN_,SCAN_COL1_IDX,_PIN)
#define EXT_SCAN_EXAMPLE_FUNCTION_LINE1         ATPASTE3(AVR32_EIC_SCAN_,SCAN_COL1_IDX,_FUNCTION)
#define EXT_SCAN_EXAMPLE_PIN_LINE2              ATPASTE3(AVR32_EIC_SCAN_,SCAN_COL2_IDX,_PIN)
#define EXT_SCAN_EXAMPLE_FUNCTION_LINE2         ATPASTE3(AVR32_EIC_SCAN_,SCAN_COL2_IDX,_FUNCTION)
//! @}


//! Structure holding the configuration parameters of the EIC module.
eic_options_t eic_options[2];

//! Global var used to switch between different LED patterns.
unsigned char key_number = 0;


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
  int line_number;
  line_number = eic_get_interrupt_pad_scan(&AVR32_EIC);
  switch(line_number)
  {
  case SCAN_COL1_IDX :
    key_number = 1;
    break;
  case SCAN_COL2_IDX :
    key_number = 2;
    break;
  }
  eic_clear_interrupt_line(&AVR32_EIC, EXT_INT_EXAMPLE_LINE1);
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
  int line_number;
  line_number = eic_get_interrupt_pad_scan(&AVR32_EIC);
  switch(line_number)
  {
  case SCAN_COL1_IDX :
    key_number = 3;
    break;
  case SCAN_COL2_IDX :
    key_number = 4;
    break;
  }
  eic_clear_interrupt_line(&AVR32_EIC, EXT_INT_EXAMPLE_LINE2);
}


/* \brief Software delay
 *
 */
static void software_delay(void)
{
  int i;
  for (i=0; i<10000; i++);
}


/* 
 * \brief This is an example on how to use an external interrupt and to generate
 * interrupt using SCAN lines.
 */
int main(void)
{
  // Enable edge-triggered interrupt.
  eic_options[0].eic_mode  = EIC_MODE_EDGE_TRIGGERED;
  // Interrupt will trigger on falling edge (this is a must-do for the keypad scan
  // feature if the chosen mode is edge-triggered).
  eic_options[0].eic_edge  = EIC_EDGE_FALLING_EDGE;
  // Initialize in synchronous mode : interrupt is synchronized to the clock
  eic_options[0].eic_async = EIC_SYNCH_MODE;
  // Set the interrupt line number.
  eic_options[0].eic_line  = EXT_INT_EXAMPLE_LINE1;


  // Enable edge-triggered interrupt.
  eic_options[1].eic_mode   = EIC_MODE_EDGE_TRIGGERED;
  // Interrupt will trigger on falling edge (this is a must-do for the keypad scan
  // feature if the chosen mode is edge-triggered).
  eic_options[1].eic_edge  = EIC_EDGE_FALLING_EDGE;
  // Initialize in synchronous mode : interrupt is synchronized to the clock
  eic_options[1].eic_async  = EIC_SYNCH_MODE;
  // Set the interrupt line number.
  eic_options[1].eic_line   = EXT_INT_EXAMPLE_LINE2;

  
  // Activate LED0 & LED1 & LED2 & LED3 pins in GPIO output mode and switch them off.
  gpio_set_gpio_pin(LED0_GPIO);
  gpio_set_gpio_pin(LED1_GPIO);
  gpio_set_gpio_pin(LED2_GPIO);
  gpio_set_gpio_pin(LED3_GPIO);
 
  // Map the interrupt lines to the GPIO pins with the right peripheral functions.
  static const gpio_map_t EIC_GPIO_MAP =
  {
    {EXT_INT_EXAMPLE_PIN_LINE1, EXT_INT_EXAMPLE_FUNCTION_LINE1},
    {EXT_INT_EXAMPLE_PIN_LINE2, EXT_INT_EXAMPLE_FUNCTION_LINE2},
    {EXT_SCAN_EXAMPLE_PIN_LINE1, EXT_SCAN_EXAMPLE_FUNCTION_LINE1},
    {EXT_SCAN_EXAMPLE_PIN_LINE2, EXT_SCAN_EXAMPLE_FUNCTION_LINE2}
  };
  gpio_enable_module(EIC_GPIO_MAP, sizeof(EIC_GPIO_MAP) / sizeof(EIC_GPIO_MAP[0]));
  // Enable GPIO pullups for the rows.
  gpio_enable_pin_pull_up(EXT_INT_EXAMPLE_PIN_LINE1);
  gpio_enable_pin_pull_up(EXT_INT_EXAMPLE_PIN_LINE2);
  
  // Disable all interrupts.
  Disable_global_interrupt();

  // Initialize interrupt vectors.
  INTC_init_interrupts();

  // Register the EIC interrupt handler to the interrupt controller.
  // eic_int_handler1 and eic_int_handler2 are the interrupt handlers to register.
  // EXT_INT_EXAMPLE_IRQ_LINE1 and EXT_INT_EXAMPLE_IRQ_LINE2 are the IRQ of the
  // interrupt handlers to register.
  // INT0 is the interrupt priority level to assign to the group of this IRQ.
  // void INTC_register_interrupt(__int_handler handler, unsigned int irq, unsigned int int_lev);
  INTC_register_interrupt(&eic_int_handler1, EXT_INT_EXAMPLE_IRQ_LINE1, INT0);
  INTC_register_interrupt(&eic_int_handler2, EXT_INT_EXAMPLE_IRQ_LINE2, INT0);

  // Init the EIC controller with the options
  eic_init(&AVR32_EIC, eic_options, 2);
  // Enable the EIC lines.
  eic_enable_lines(&AVR32_EIC, (1<<eic_options[1].eic_line)|(1<<eic_options[0].eic_line));
  // Enable the interrupt for each EIC line.
  eic_enable_interrupt_lines(&AVR32_EIC, (1<<eic_options[1].eic_line)|(1<<eic_options[0].eic_line));
  // Enable the keypad scanning feature and configure the scan rate to 18ms (== pow(2,(10+1))*RCOsc).
  eic_enable_interrupt_scan(&AVR32_EIC,10);

  // Enable all interrupts.
  Enable_global_interrupt();


  // Loop infinitely, toggling the leds depending on the latest pressed key.
  while(1)
  {
    switch (key_number)
    {
      case 1 :
        gpio_clr_gpio_pin(LED0_GPIO);
        gpio_set_gpio_pin(LED1_GPIO);
        gpio_set_gpio_pin(LED2_GPIO);
        gpio_set_gpio_pin(LED3_GPIO);
        key_number = 0;
        software_delay();
        break;
      case 2 :
        gpio_set_gpio_pin(LED0_GPIO);
        gpio_clr_gpio_pin(LED1_GPIO);
        gpio_set_gpio_pin(LED2_GPIO);
        gpio_set_gpio_pin(LED3_GPIO);
        key_number = 0;
        software_delay();
        break;
      case 3 :
        gpio_set_gpio_pin(LED0_GPIO);
        gpio_set_gpio_pin(LED1_GPIO);
        gpio_clr_gpio_pin(LED2_GPIO);
        gpio_set_gpio_pin(LED3_GPIO);
        key_number = 0;
        software_delay();
        break;
      case 4 :
        gpio_set_gpio_pin(LED0_GPIO);
        gpio_set_gpio_pin(LED1_GPIO);
        gpio_set_gpio_pin(LED2_GPIO);
        gpio_clr_gpio_pin(LED3_GPIO);
        key_number = 0;
        software_delay();
        break;
      default :
        gpio_set_gpio_pin(LED0_GPIO);
        gpio_set_gpio_pin(LED1_GPIO);
        gpio_set_gpio_pin(LED2_GPIO);
        gpio_set_gpio_pin(LED3_GPIO);
        break;
    }
  }

}
