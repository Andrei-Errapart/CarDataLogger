/* This source file is part of the ATMEL AT32UC3A-SoftwareFramework-1.1.1 Release */

/*This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief MACB example driver for EVK1100 board.
 *
 * This file provides a useful example for the MACB interface on AVR32 devices.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with a MACB module can be used.
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
 * This documents data structures, functions, variables, defines, enums, and
 * typedefs in the software. <BR>It also gives an example of the usage of the
 * Ethernet MAC 10/100 (MACB). <BR>It answers to incoming ping request.
 * <BR>Default setup : EVK1100 IP address= 192.168.0.2 and PC IP address = 192.168.0.1<BR>Use a non crossed RJ45 cable.
 *
 * \section files Main Files
 * - macb.c : MACB driver
 * - macb.h : MACB header file
 * - macb_example.c : MACB code example
 * - conf_eth.h : ethernet configuration (useful options : Use interrupts, HW and IP address, etc)
 *
 * \section compinfo Compilation Info
 * This software was written for the GNU GCC for AVR32 and IAR Systems compiler
 * for AVR32. Other compilers may or may not work.
 *
 * \section deviceinfo Device Info
 * All AVR32 devices with a MACB module can be used. This example has been tested
 * with the following setup:<BR>
 * - EVK1100 evaluation kit
 *
 * \section setupinfo Setup Information
 * <BR>CPU speed: <i> 48 MHz </i>
 * - Connect the USART_1 to your serial port via a standard RS-232 D-SUB9 cable
 * - Set the following settings in your terminal of choice: 57600 8N1
 *
 * \section contactinfo Contact Information
 * For further information, visit
 * <A href="http://www.atmel.com/products/AVR32/">Atmel AVR32</A>.\n
 * Support and FAQ: http://support.atmel.no/
 */

#include <stdio.h>
#include <string.h>

#include <avr32/io.h>
#include "print_funcs.h"
#include "usart.h"
#include "board.h"
#include "macb.h"
#include "gpio.h"
#include "compiler.h"
#include "conf_eth.h"
#include "flashc.h"
#include "pm.h"
#include "intc.h"

/*! initial sequence number for ICMP request and reply */
#define SEQ_NUM_START 0x2546

/*! sequence number for ICMP request and reply */
unsigned short seqnum = SEQ_NUM_START;

/*! buffer for sending packets */
const char local_ipaddr[4] = {ETHERNET_CONF_IPADDR0,ETHERNET_CONF_IPADDR1,ETHERNET_CONF_IPADDR2,ETHERNET_CONF_IPADDR3};

/*! define the ARP global frame */
const char ARP_FRAME[42] = {
0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
ETHERNET_CONF_ETHADDR0,ETHERNET_CONF_ETHADDR1,ETHERNET_CONF_ETHADDR2,ETHERNET_CONF_ETHADDR3,ETHERNET_CONF_ETHADDR4,ETHERNET_CONF_ETHADDR5,
0x08,0x06,0x00,0x01,0x08,0x00,0x06,0x04,0x00,0x01,
ETHERNET_CONF_ETHADDR0,ETHERNET_CONF_ETHADDR1,ETHERNET_CONF_ETHADDR2,ETHERNET_CONF_ETHADDR3,ETHERNET_CONF_ETHADDR4,ETHERNET_CONF_ETHADDR5,
ETHERNET_CONF_IPADDR0,ETHERNET_CONF_IPADDR1,ETHERNET_CONF_IPADDR2,ETHERNET_CONF_IPADDR3,
0x00,0x00,0x00,0x00,0x00,0x00,
ETHERNET_CONF_GATEWAY_ADDR0,ETHERNET_CONF_GATEWAY_ADDR1,ETHERNET_CONF_GATEWAY_ADDR2,ETHERNET_CONF_GATEWAY_ADDR3
};
                           
/*! buffer for sending packets */
char data[ETHERNET_CONF_TX_BUFFER_SIZE];
/* The IP and Ethernet addresses are read from the header files. */
char hwaddr[ 6 ] = { ETHERNET_CONF_ETHADDR0,ETHERNET_CONF_ETHADDR1,ETHERNET_CONF_ETHADDR2,ETHERNET_CONF_ETHADDR3,ETHERNET_CONF_ETHADDR4,ETHERNET_CONF_ETHADDR5 };


/**
 * \brief Checksum routine for Internet Protocol family headers
 *
 * \param  addr   address of data to compute checksum
 * \param  len    length of data to compute checksum
 *
 * \return unsigned short checksum computed
 */
unsigned short in_cksum(unsigned short * addr, int len)
{
int nleft, sum;
unsigned short *w;
union {
  unsigned short us;
  unsigned char  uc[2];
} last;
unsigned short answer;

  nleft = len;
  sum = 0;
  w = addr;

  /*
   * Algorithm is simple, using a 32 bit accumulator (sum) :
   * add sequential 16 bit words to it, and at the end, fold back all the
   * carry bits from the top 16 bits into the lower 16 bits.
   */
  while (nleft > 1)
  {
    sum += *w++;
    nleft -= 2;
  }

  // mop up an odd byte, if necessary
  if (nleft == 1)
  {
    last.uc[0] = *(unsigned char *)w;
    last.uc[1] = 0;
    sum += last.us;
  }

  // add back carry outs from top 16 bits to low 16 bits
  sum = (sum >> 16) + (sum & 0xffff);     // add hi 16 to low 16
  sum += (sum >> 16);                     // add carry
  answer = ~sum;                          // truncate to 16 bits
  return(answer);
}

/*!
 * \brief function to send PING response to pkt->data->host
 *
 * \param pkt packet to manage
 */
void macb_example_send_ping_response(macb_packet_t * pkt)
{
char ipaddr[4];
unsigned short chksum;

  // swap sender & receiver HW address
  memmove(pkt->data, pkt->data+6, 6);
  memcpy(pkt->data+6, hwaddr, 6);
  // swap sender & receiver IP address
  memcpy(ipaddr, pkt->data+26, 4);
  memmove(pkt->data+26, pkt->data+30, 4);
  memcpy(pkt->data+30, ipaddr, 4);
  // set seq num
  pkt->data[18] = MSB(seqnum);
  pkt->data[19] = LSB(seqnum);
  if (++seqnum >= 0xFF00) seqnum = SEQ_NUM_START;
  // reset checksum before computation
  pkt->data[24] = 0;
  pkt->data[25] = 0;
  // compute IP checksum
  chksum = in_cksum((unsigned short *)&pkt->data[14], 20);
  // set IP checksum
  pkt->data[24] = MSB(chksum);
  pkt->data[25] = LSB(chksum);
  // set reply bit
  pkt->data[34] = 0;
  // reset checksum before computation
  pkt->data[36] = 0;
  pkt->data[37] = 0;
  // compute ICMP checksum
  chksum = in_cksum((unsigned short *)&pkt->data[34], (pkt->len - 34));
  // set ICMP checksum
  pkt->data[36] = MSB(chksum);
  pkt->data[37] = LSB(chksum);
  // send request, nothing else to send
  lMACBSend(&AVR32_MACB, pkt->data, pkt->len, TRUE);
}

/*!
 * \brief callback to manage packets reception
 * \param pkt packet to manage
 */
void macb_example_receive_packet(macb_packet_t *pkt)
{
  char ipaddr[4];

  // if it is a ARP frame, answer it
  if ((pkt->data[12] == 0x08) && (pkt->data[13] == 0x06))
  {
    // ARP request
    if ((pkt->data[20] == 0x00) && (pkt->data[21] == 0x01))
    {
      print_dbg("ARP Request received...\r\n");
      // swap sender & receiver address
      memmove(pkt->data, pkt->data+6, 6);
      memcpy(pkt->data+6, hwaddr, 6);
      // ARP Response
      pkt->data[21] = 0x02;
      // swap sender & receiver parameters
      memcpy(ipaddr, pkt->data+38, 4);
      memmove(pkt->data+32, pkt->data+22, 10);
      memcpy(pkt->data+22, hwaddr, 6);
      memcpy(pkt->data+28, ipaddr, 4);
      // send response, nothing else to send
      lMACBSend(&AVR32_MACB, pkt->data, pkt->len, TRUE);
    }
    // ARP response
    else if ((pkt->data[20] == 0x00) && (pkt->data[21] == 0x02))
    {
      print_dbg("ARP Response received...\r\n");
    }    
    // unknown frame, loop it back
    else
    {
      print_dbg("Unimplemented ARP packet...\r\n");
    }
  }
  // ICMP protocol
  else if (pkt->data[23] == 0x01)
  {
    // if it is our IP address
    if (!memcmp(&pkt->data[30], local_ipaddr, 4))
    {
      if (pkt->data[34] == 0x08) // PING Request
      {
        print_dbg("PING Request received...\r\n");
        macb_example_send_ping_response(pkt);   
      }
      else
      {
        print_dbg("Unimplemented ICMP packet...\r\n");
      }
    }
  }
  else
  {
    print_dbg("Unknown packet...\r\n");
  }
}

/*! \brief Send an ARP request to host
 *
 */
static void macb_example_send_ARP_request(void)
{
macb_packet_t request_pkt;

  request_pkt.data = data;
  // Prepare ARP frame for host
  memcpy(request_pkt.data, ARP_FRAME, 42);
  request_pkt.len = 42;
#if CONF_MACB_VERBOSE
  print_dbg("Sending ARP Request...\r\n");
#endif
  // send response, nothing else to send
  lMACBSend(&AVR32_MACB, request_pkt.data, request_pkt.len, TRUE);  
}

/*!
 * \brief start a PLL @ 48Mhz to run with
 */
void local_start_pll(void)
{
volatile avr32_pm_t* pm = &AVR32_PM;

  // Switch the main clock to OSC0
  pm_switch_to_osc0(pm, FOSC0, OSC0_STARTUP);
  // Setup PLL0 on OSC0, mul=15 ,no divisor, lockcount=16, ie. 12Mhzx16 = 192MHz output
  pm_pll_setup(pm,  // volatile avr32_pm_t* pm
               0,   // unsigned int pll
               15,   // unsigned int mul
               1,   // unsigned int div, Sel Osc0/PLL0 or Osc1/Pll1
               0,   // unsigned int osc
               16); // unsigned int lockcount
  // set PLL options to run @ 96 Mhz
  pm_pll_set_option(pm, // volatile avr32_pm_t *pm
                    0,  // unsigned int pll
                    0,  // unsigned int  pll_freq
                    1,  // unsigned int  pll_div2
                    0); // unsigned int  pll_wbwdisable
  // Enable PLL0
  pm_pll_enable(pm,0);
  // Wait for PLL0 locked
  pm_wait_for_pll0_locked(pm) ;
  // Setup generic clock number 0 on PLL, with OSC0/PLL0, no divisor
  pm_gc_setup(pm,
              0,
              1, // Use Osc (=0) or PLL (=1)
              0, // Sel Osc0/PLL0 or Osc1/Pll1
              0,
              0);
  // Enable Generic clock 0*/
  pm_gc_enable(pm, 0);
  // set divider to 8 for PBA bus and 2 for PBB
  pm_cksel(pm,1,2,1,0,1,0);
  // one wait state at 48 Mhz
  flashc_set_wait_state(1);
  /* Output the clock to a gpio(PA7) */
  gpio_enable_module_pin(AVR32_PM_GCLK_0_0_PIN, AVR32_PM_GCLK_0_0_FUNCTION);
  // switch to clock
  pm_switch_to_clock(pm, AVR32_PM_MCCTRL_MCSEL_PLL0);
}

/*!
 * \brief main function : do init and loop (poll if configured so)
 */
int main(int argc, char **argv)
{
unsigned long len, i = 0;
macb_packet_t recvd_pkt;
static const gpio_map_t MACB_GPIO_MAP =
{
  {AVR32_MACB_MDC_0_PIN,    AVR32_MACB_MDC_0_FUNCTION   },
  {AVR32_MACB_MDIO_0_PIN,   AVR32_MACB_MDIO_0_FUNCTION  },
  {AVR32_MACB_RXD_0_PIN,    AVR32_MACB_RXD_0_FUNCTION   },
  {AVR32_MACB_TXD_0_PIN,    AVR32_MACB_TXD_0_FUNCTION   },
  {AVR32_MACB_RXD_1_PIN,    AVR32_MACB_RXD_1_FUNCTION   },
  {AVR32_MACB_TXD_1_PIN,    AVR32_MACB_TXD_1_FUNCTION   },
  {AVR32_MACB_TX_EN_0_PIN,  AVR32_MACB_TX_EN_0_FUNCTION },
  {AVR32_MACB_RX_ER_0_PIN,  AVR32_MACB_RX_ER_0_FUNCTION },
  {AVR32_MACB_RX_DV_0_PIN,  AVR32_MACB_RX_DV_0_FUNCTION },
  {AVR32_MACB_TX_CLK_0_PIN, AVR32_MACB_TX_CLK_0_FUNCTION}
};

  // start a PLL
  local_start_pll();

  // init the system interrupts
  INTC_init_interrupts();

  // init debug serial line
  init_dbg_rs232(FOSC0);

  // display welcome
  print_dbg("\x1B[2J\x1B[H\r\n MACB Example\r\n");

  // Assign GPIO to MACB
  gpio_enable_module(MACB_GPIO_MAP, sizeof(MACB_GPIO_MAP) / sizeof(MACB_GPIO_MAP[0]));

  // initialize MACB & Phy Layers
  if (xMACBInit(&AVR32_MACB) == FALSE )
  {
    gpio_clr_gpio_pin(LED4_GPIO);
    while(1);	
  }
  
  gpio_clr_gpio_pin(LED5_GPIO);
 
  // do a loop, and display stats or quit
  while (1)
  {
    // Loop while no data are pending.
    vMACBWaitForInput(100);
    // Obtain the size of the packet.
    len = ulMACBInputLength();
    if( len != 0)
    {
      // Let the driver know we are going to read a new packet.
      vMACBRead( NULL, 0, 0 );
      // Read enough bytes to fill this buf.
      vMACBRead( data, 128, len );
      recvd_pkt.data = data;
      recvd_pkt.len = len;
      macb_example_receive_packet(&recvd_pkt);
    }
    if (++i >= 400)
    {
      i = 0;
      // send ARP request to host
      macb_example_send_ARP_request();
    }    
  }
}

