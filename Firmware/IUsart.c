/**
vim: ts=4
vim: shiftwidth=4
*/
#include "project.h"
#include "IUsart.h"
#include "Display.h"
#include "Utils.h"

#include <stdbool.h>

typedef struct {
	volatile avr32_usart_t*	usart;
	IUsart_RxCallback		rx_callback;
} IUsart_Port;

static IUsart_Port	ports[2] = {
	{ &AVR32_USART0, NULL },
	{ &AVR32_USART1, NULL }
};

//*******************************************************************
#define do_isr(idx) do { 													\
	IUsart_Port*			port = &ports[idx];								\
	const unsigned long 	status = port->usart->csr & port->usart->imr;	\
	if (status & AVR32_USART_CSR_RXRDY_MASK) {								\
		const char c = port->usart->rhr;									\
		if (port->rx_callback != NULL) {									\
			port->rx_callback(c);											\
		}																	\
	}																		\
} while(0)

//*******************************************************************
/** USART0 interrupt service routine. */
__attribute__((__interrupt__))
void usart0_isr( void )
{
	do_isr(0);
}

//*******************************************************************
/** USART1 service routine. */
__attribute__((__interrupt__))
void usart1_isr( void )
{
	do_isr(1);
}

//*******************************************************************
static const gpio_map_t USART_0_GPIO_MAP =
{
	{AVR32_USART0_RXD_0_PIN, AVR32_USART0_RXD_0_FUNCTION},
	{AVR32_USART0_TXD_0_PIN, AVR32_USART0_TXD_0_FUNCTION}
};

static const gpio_map_t USART_1_GPIO_MAP =
{
	{AVR32_USART1_RXD_0_PIN, AVR32_USART1_RXD_0_FUNCTION},
	{AVR32_USART1_TXD_0_PIN, AVR32_USART1_TXD_0_FUNCTION}
};

//*******************************************************************
static inline int
myabs(
	const int	x
)
{
	return x<0 ? -x : x;
}

//*******************************************************************
/** Minimizes baud rate error by finding optimal UART settings.

Note: overclock is fixed to 16x in this version.

\param[in]	BaudRate	Desired baud rate.
\param[out]	cd			Optimal Clock Divider
\param[out]	fp			Optimal Fractional Part
\param[out]	over		Optimal Overclock flag, 0=16x, 1=8x.
\return	UART baud rate error, promills (1/1000th).
 */
static unsigned int
FindOptimalUsartSettings(
	const int				BaudRate,
	unsigned int*			cd,
	unsigned int*			fp,
	unsigned int*			over
	)
{
	int		trial_fp = 0;
	int		trial_over = 0;
	int		trial_cd = F_PBA / (8*(2-trial_over) * BaudRate);
	int		trial_error = myabs((2-trial_over) * (8*trial_cd + trial_fp)*BaudRate - F_PBA);

	int		best_cd = trial_cd;
	int		best_fp = trial_fp;
	int		best_over = trial_over;
	int		best_error = trial_error;

	for (trial_over=0; trial_over<1; ++trial_over) {
		const int		cd0 = F_PBA / (8*(2-trial_over) * BaudRate);
		const int		cd1 = cd0 + 1;
		for (trial_cd = cd0; trial_cd<=cd1; ++trial_cd) {
			for (trial_fp=0; trial_fp<8; ++trial_fp) {
				trial_error = myabs((2-trial_over) * (8*trial_cd+trial_fp)*BaudRate - F_PBA);
				if (trial_error < best_error) {
					best_cd = trial_cd;
					best_fp = trial_fp;
					best_over = trial_over;
					best_error = trial_error;
				}
			}
		}
	}

	*cd = best_cd;
	*fp = best_fp;
	*over = best_over;

	return (best_error + (F_PBA/2000)) / (F_PBA / 1000);
}

#if (0)
//*******************************************************************
static void
Display_UsartSettings(
	const unsigned int		cd,
	const unsigned int		over,
	const unsigned int		fp,
	const unsigned int		error
)
{
	char	xbuf[40];
	char	ibuf[20];

	xbuf[0] = 0;
	itoa(cd, xbuf, 10);
	mystrcat(xbuf, " ");
	mystrcat(xbuf, itoa(over, ibuf, 10));
	mystrcat(xbuf, " ");
	mystrcat(xbuf, itoa(fp, ibuf, 10));
	mystrcat(xbuf, " ");
	mystrcat(xbuf, itoa(error, ibuf, 10));

	Display_Error(xbuf);
}
#endif

//*******************************************************************
void IUsart_Init(
	const IUsart			UsartNr,
	const IUsart_Mode		Mode,
	const unsigned int		InterruptPriority,
	const unsigned int		BaudRate,
	const IUsart_RxCallback	RxCallback
)
{
	volatile avr32_usart_t*	usart = ports[UsartNr].usart;
	unsigned int			cd; /* Clock Divider. */
	unsigned int			fp; /* Fractional Part. */
	unsigned int			over; /* Usart Overclock flag. */
	unsigned int			baudrate_error;

	ports[UsartNr].rx_callback = RxCallback;

	/* Disable all USART interrupt sources to begin... */
	usart->idr = 0xFFFFFFFF;

	/* Reset mode and other registers that could cause unpredictable
	behaviour after reset */
	usart->mr = 0; /* Reset Mode register. */
	usart->rtor = 0; /* Reset Receiver Time-out register. */
	usart->ttgr = 0; /* Reset Transmitter Timeguard register. */

	/* Shutdown RX and TX, reset status bits, reset iterations in CSR, reset NACK
	and turn off DTR and RTS */
	usart->cr =
		AVR32_USART_CR_RSTRX_MASK   |
		AVR32_USART_CR_RSTTX_MASK   |
		AVR32_USART_CR_RXDIS_MASK   |
		AVR32_USART_CR_TXDIS_MASK   |
		AVR32_USART_CR_RSTSTA_MASK  |
		AVR32_USART_CR_RSTIT_MASK   |
		AVR32_USART_CR_RSTNACK_MASK |
		AVR32_USART_CR_DTRDIS_MASK  |
		AVR32_USART_CR_RTSDIS_MASK;

	// Assign GPIO to USART.
	if (UsartNr == 0) {
		gpio_enable_module(USART_0_GPIO_MAP, sizeof(USART_0_GPIO_MAP) / sizeof(USART_0_GPIO_MAP[0]));
		if (Mode == IUsart_RS485) {
			gpio_enable_module_pin(AVR32_USART0_RTS_0_PIN, AVR32_USART0_RTS_0_FUNCTION);
		}
	} else {
		gpio_enable_module(USART_1_GPIO_MAP, sizeof(USART_1_GPIO_MAP) / sizeof(USART_1_GPIO_MAP[0]));
		if (Mode == IUsart_RS485) {
			gpio_enable_module_pin(AVR32_USART1_RTS_0_PIN, AVR32_USART1_RTS_0_FUNCTION);
		}
	}

	baudrate_error = FindOptimalUsartSettings(BaudRate, &cd, &fp, &over);
	if (over) {
		usart->mr |= (1<<AVR32_USART_MR_OVER_OFFSET);
	} else {
		usart->mr &= ~(1<<AVR32_USART_MR_OVER_OFFSET);
	}
	usart->brgr = (cd << AVR32_USART_BRGR_CD_OFFSET) | (fp << AVR32_USART_BRGR_FP_OFFSET);

	/* Set the USART Mode register: Mode=Normal(0), Clk selection=MCK(0),
	CHRL=8,  SYNC=0(asynchronous), PAR=None, NBSTOP=1, CHMODE=0, MSBF=0,
	MODE9=0, CKLO=0, OVER(previously done when setting the baudrate),
	other fields not used in this mode. */
	usart->mr |=
		((8-5) << AVR32_USART_MR_CHRL_OFFSET  ) |
		(   4  << AVR32_USART_MR_PAR_OFFSET   ) |
		(   1  << AVR32_USART_MR_NBSTOP_OFFSET);

	if (Mode == IUsart_RS485) {
		// Clear previous mode.
		usart->mr &= ~AVR32_USART_MR_MODE_MASK;
		// Set RS485 mode.
		usart->mr |= USART_MODE_RS485 << AVR32_USART_MR_MODE_OFFSET;
	}

	/* Write the Transmit Timeguard Register */
	usart->ttgr = 0;

	// Register the USART interrupt handler to the interrupt controller and
	// enable the USART interrupt.
	switch (UsartNr) {
	case IUsart0:
		INTC_register_interrupt(&usart0_isr, AVR32_USART0_IRQ, InterruptPriority);
		break;
	case IUsart1:
		INTC_register_interrupt(&usart1_isr, AVR32_USART1_IRQ, InterruptPriority);
		break;
	}

	/* Enable USART interrupt sources (but not Tx for now)... */
	usart->ier = AVR32_USART_IER_RXRDY_MASK;

	/* Enable receiver and transmitter... */
	usart->cr |= AVR32_USART_CR_RXEN_MASK | AVR32_USART_CR_TXEN_MASK;
}

//*******************************************************************
void IUsart_Write(
	IUsart					UsartNr,
	const char				C
)
{
	volatile avr32_usart_t*	usart = ports[UsartNr].usart;
	while (!usart_tx_ready(usart))
		;
	usart->thr = C;
}

