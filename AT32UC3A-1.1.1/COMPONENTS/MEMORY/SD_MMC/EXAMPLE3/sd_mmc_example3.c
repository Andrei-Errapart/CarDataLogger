/*This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief SD/MMC card driver example application.
 *
 * This file gives an example of using the SD/MMC card driver.
 * If a SD/MMC card is detected, a message is sent to user
 * after that it copies data from SD/MMC to RAM using the pdca.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with an SPI module can be used.
 *                       The example is written for UC3 and EVK1100.
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


#include <avr32/io.h>
#include "compiler.h"
#include "board.h"
#include "pm.h"
#include "gpio.h"
#include "spi.h"
#include "sd_mmc.h"
#include "usart.h"
#include "print_funcs.h"
#include "pdca.h"
#if __GNUC__
#  include "intc.h"
#endif


/* it is only a dummy variable*/
const char ascii_anim[] =
#include "ascii_anim2.h"
;


/*! \brief PBA clock frequency (Hz) */
#define PBA_HZ                FOSC0

/*! \brief Number of bits in each SPI package*/
#define SPI_BITS              8

/*! \brief SPI master speed in Hz */
#define SPI_MASTER_SPEED      12000000

/*! \brief Number of bytes in the receive buffer when operating in slave mode */
#define BUFFERSIZE            64

#if BOARD==EVK1100
#define AVR32_PDCA_CHANNEL_USED_RX AVR32_PDCA_PID_SPI1_RX
#define AVR32_PDCA_CHANNEL_USED_TX AVR32_PDCA_PID_SPI1_TX
#elif BOARD==EVK1101
#define AVR32_PDCA_CHANNEL_USED_RX AVR32_PDCA_PID_SPI0_RX
#define AVR32_PDCA_CHANNEL_USED_TX AVR32_PDCA_PID_SPI0_TX
#else
#  error No known AVR32 board defined
#endif

/* Assigning DMA channels*/
volatile int pdca_channel_spi_RX = 1;// In the example we will use the pdca channel 1. There is 17 possible pdca channel in AT32UC3A0512.
volatile int pdca_channel_spi_TX = 2;// In the example we will use the pdca channel 2. There is 17 possible pdca channel in AT32UC3A0512.
volatile int end_of_transfer;

volatile char buffer[1000];

void wait()
{
  int i;
  for(i = 0 ; i < 5000; i++);
}


/********************************************************************************/
/*                   Interrupt handler                                         */
/*******************************************************************************/

/* interruption handler to notify if the Data reception from flash is
 * over, in this case lunch the Memory(Buffer) to USART transfer and
 * disable interrupt*/

#if __GNUC__
__attribute__((__interrupt__))
#elif __ICCAVR32__
__interrupt
#endif
static void pdca_int_handler(void)
{
  // Disable all interrupts.
  Disable_global_interrupt();

  /*Disable interrupt channel*/
  pdca_disable_interrupt_transfer_complete(pdca_channel_spi_RX);

  sd_mmc_read_close_PDCA();//unselects the Pattern memory.
  wait();
  /*Disable unnecessary channel*/
  pdca_disable(pdca_channel_spi_TX);
  pdca_disable(pdca_channel_spi_RX);

  // Enable all interrupts.
  Enable_global_interrupt();

  end_of_transfer = TRUE;
}


/*init DMA resources necessary for the SPI transfer*/
void READER_Init_PDCA(void)
{
  int i;

  /*this channel is used for data reception from the SPI*/
  pdca_channel_options_t pdca_options_SPI_RX ={ /* pdca channel options */

    .addr = (unsigned int)(buffer),
   /* memory address. We take here the address of the string ascii_anim. This string is located in the file ascii_anim.h */

    .size = 513,  /* transfer counter: here the size of the string */

    .r_addr = 0,           /* next memory address after 1st transfer complete */

    .r_size = 0,     /* next transfer counter not used here */

    .pid = AVR32_PDCA_CHANNEL_USED_RX,  /* select peripheral ID - data are on reception from SPI1 RX line */

    .mode = PDCA_MODE_BYTE, /* select size of the transfer: 8,16,32 bits */
  };


  /*this channel is used to activate the clock of the SPI by sending a dummy variables*/
  pdca_channel_options_t pdca_options_SPI_TX ={ /* pdca channel options */

    .addr = (unsigned int)(&ascii_anim),
   /* memory address. We take here the address of the string ascii_anim. This string is located in the file ascii_anim.h */

    .pid = AVR32_PDCA_CHANNEL_USED_TX,  /* select peripheral ID - data are on reception from SPI1 RX line */

    .size = 512,  /* transfer counter: here the size of the string */

    .r_addr = 0,           /* next memory address after 1st transfer complete */

    .r_size = 0,     /* next transfer counter not used here */

    .mode = PDCA_MODE_BYTE, /* select size of the transfer: 8,16,32 bits */
  };


  /* Init pdca transmission channel*/
  pdca_init_channel(pdca_channel_spi_TX, &pdca_options_SPI_TX);

  /*Init pdca Reception channel*/
  pdca_init_channel(pdca_channel_spi_RX, &pdca_options_SPI_RX);

  #if __GNUC__
  /*! \brief Enable pdca transfer interrupt when completed*/
  INTC_register_interrupt(&pdca_int_handler, AVR32_PDCA_IRQ_1, INT1);  // pdca_channel_spi1_RX = 1
  INTC_register_interrupt(&pdca_int_handler, AVR32_PDCA_IRQ_2, INT1);  // pdca_channel_spi1_TX = 2

#endif

  /*! \brief Enable pdca transfer interrupt when completed*/
  //pdca_enable_interrupt_transfer(pdca_channel_spi1_TX);
  //pdca_enable_interrupt_transfer(pdca_channel_spi1_RX);



  print_dbg("lunching unnecessary transfer\n");
  end_of_transfer = FALSE;

  if (sd_mmc_read_open_PDCA( 0 )  == OK)
  {
      //enable interrupt channel
      pdca_enable_interrupt_transfer_complete(pdca_channel_spi_RX);

      // enable the transfer
      pdca_enable(pdca_channel_spi_RX);
      pdca_enable(pdca_channel_spi_TX);
  }
  else
  print_dbg("unable to select SD_MMC for the unnecessary transfert\n");

  while( !end_of_transfer ) ;//wait the end of transfer
  print_dbg("End of the unnecessary transfert\n");


  for( i = 0; i < 50; i++)
      {
        print_dbg_hex( (U8)(*(buffer + i)));
      }

}


/*! \brief Main function. Executing starts here.
 */
int main(void)
{
  int i,j;
  U8 status;
  spi_options_t spiOptions =
  {
    .reg          = 1,
    .baudrate     = SPI_MASTER_SPEED, // Defined in conf_at45dbx.h
    .bits         = SPI_BITS,         // Defined in conf_at45dbx.h
    .spck_delay   = 0,
    .trans_delay  = 0,
    .stay_act     = 1,
    .spi_mode     = 0,
    .fdiv         = 0,
    .modfdis      = 1
  };

  /* Switch the main clock to the external oscillator 0 */
  pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);
  init_dbg_rs232(PBA_HZ);

  //start test
  print_dbg("\n*******INIT SD/MMC...");

  /* Initialize SPI as a Master */
  status = sd_mmc_init(spiOptions, PBA_HZ);
  status = sd_mmc_mem_check();
  if (status == OK )
  {
     print_dbg("\n******Card detected!");
  }
  sd_mmc_get_capacity();
  print_dbg("\n******Capacity = ");
  print_dbg_hex(capacity);
  print_dbg(" Bytes\n");

  // Enable all interrupts.
  Enable_global_interrupt();


  READER_Init_PDCA();



   /*! \brief add the adddress of memory buffer*/
        pdca_set_memory_add( pdca_channel_spi_RX,
                           ((unsigned int)(buffer)) ,
                           513);

        pdca_set_memory_add( pdca_channel_spi_TX,
                           (unsigned int)(&ascii_anim),
                           512);//send dummy to activate the clock


  for(j = 1; j < 2; j++)
  {

    end_of_transfer = FALSE;
    if(sd_mmc_read_open_PDCA (j) == OK)
    {

        print_dbg("\n******Transfer  number = ");
        print_dbg_hex(j);
        print_dbg("\n");

       //enable interrupt channel
        pdca_enable_interrupt_transfer_complete(pdca_channel_spi_RX);

        // enable the transfer
        pdca_enable(pdca_channel_spi_RX);
        pdca_enable(pdca_channel_spi_TX);

       while(!end_of_transfer);

      for( i = 0; i < 50; i++)
      {
        print_dbg_hex( (U8)(*(buffer + i)));
      }
    }
    else
    {
      print_dbg("* ** ***  **** unable to open memory \n");
    }

     /*! \brief add the adddress of memory buffer*/
        pdca_set_memory_add( pdca_channel_spi_RX,
                           ((unsigned int)(buffer)) ,
                           513);

        pdca_set_memory_add( pdca_channel_spi_TX,
                           (unsigned int)(&ascii_anim),
                           512);//send dummy to activate the clock
  }

  while (1);
}
