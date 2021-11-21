/* This source file is part of the ATMEL AT32UC3A-SoftwareFramework-1.1.1 Release */

/*This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief PWM example driver for AVR32 UC3.
 *
 * This file provides an example for the PWM on AVR32 UC3 devices.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with a PWM module can be used.
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
 * This is the documentation for the data structures, functions, variables, defines, enums, and
 * typedefs for the PWM driver. <BR>It also gives an example of the usage of the
 * PWM on UC3 products.
 * This example shows how to configure a PWM and output it on a GPIO.\n
 * <b>Operating mode: </b>Check PWM0 pin with an oscilloscope, you should see a
 * PWM frequency of 22.5 Hz with a PWM duty cycle of 3/4.
 * On EVK1100, PWM0 is pin number 51 (PB19) with AT32UC3A0512 in QFP144 package.
 * \note On EVK1100, one of the bicolor leds that LED5 is made of is connected to
 * PB19. Thus, you should also see LED5 red-blink.
 *
 * \section compinfo Compilation Info
 * This software was written for the GNU GCC for AVR32 and IAR Systems compiler
 * for AVR32. Other compilers may or may not work.
 *
 * \section deviceinfo Device Info
 * All AVR32 devices with a PWM module can be used. This example has been tested
 * with the following setup:<BR>
 * <ul><li>EVK1100 evaluation kit</ul>
 *
 * \section setupinfo Setup Information
 * CPU speed: <i> Internal RC oscillator (about 115200 Hz). </i>
 *
 * \section contactinfo Contact Info
 * For more info about Atmel AVR32 visit<BR>
 * <A href="http://www.atmel.com/products/AVR32/" >Atmel AVR32</A><BR>
 * Support and FAQ: http://support.atmel.no/
 */


#include <avr32/io.h>
#include "pwm.h"
#include "gpio.h"


/*! \brief Main function. Execution starts here.
 *
 *  \return 0 on success
 */
int main()
{
  pwm_opt_t pwm_opt;                // PWM option config.
  avr32_pwm_channel_t pwm_channel;  // One channel config.
  // The channel number and instance is used in several functions.
  // It's defined as local variable for ease-of-use.
  unsigned int channel_id;

  channel_id = 0;
  gpio_enable_module_pin(AVR32_PWM_PWM_0_PIN, AVR32_PWM_PWM_0_FUNCTION);

  // PWM controller configuration.
  pwm_opt.diva = AVR32_PWM_DIVA_CLK_OFF;
  pwm_opt.divb = AVR32_PWM_DIVB_CLK_OFF;
  pwm_opt.prea = AVR32_PWM_PREA_MCK;
  pwm_opt.preb = AVR32_PWM_PREB_MCK;

  pwm_init(&pwm_opt);

  pwm_channel.CMR.calg = PWM_MODE_LEFT_ALIGNED;       // Channel mode.
  pwm_channel.CMR.cpol = PWM_POLARITY_LOW;            // Channel polarity.
  pwm_channel.CMR.cpd = PWM_UPDATE_DUTY;              // Not used the first time.
  pwm_channel.CMR.cpre = AVR32_PWM_CPRE_MCK_DIV_256;  // Channel prescaler.
  pwm_channel.cdty = 5;   // Channel duty cycle, should be < CPRD.
  pwm_channel.cprd = 20;  // Channel period.
  pwm_channel.cupd = 0;   // Channel update is not used here.
  // With these settings, the output waveform period will be :
  // (115200/256)/20 == 22.5Hz == (MCK/prescaler)/period, with MCK == 115200Hz,
  // prescaler == 256, period == 20.

  pwm_channel_init(channel_id, &pwm_channel); // Set channel configuration to channel 0.

  pwm_start_channels(1 << channel_id);  // Start channel 0.

  while(1);

  // Stop channel output.
  pwm_stop_channels(1 << channel_id); // Stop channel 0.

  return 0;
}
