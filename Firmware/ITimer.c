/**
vim: ts=4
vim: shiftwidth=4
*/
#include "project.h"
#include "ITimer.h"


#define	NCHANNELS	3

static ITimer_Callback	timer_callback_func[NCHANNELS] = {0, 0, 0};

#define	do_tc_irq(index)							\
	do {											\
		/* clear the interrupt flag */				\
		AVR32_TC.channel[index].sr;					\
		if (timer_callback_func[index]!= NULL) {	\
			timer_callback_func[index]();			\
		}											\
	} while (0)

//*******************************************************************
__attribute__((__interrupt__)) 
static void tc_irq0( void )
{
	do_tc_irq(0);
}

//*******************************************************************
__attribute__((__interrupt__)) 
static void tc_irq1( void )
{
	do_tc_irq(1);
}

//*******************************************************************
__attribute__((__interrupt__)) 
static void tc_irq2( void )
{
	do_tc_irq(2);
}

//*******************************************************************
static const tc_interrupt_t TC_INTERRUPT =
{
	.etrgs = 0,
	.ldrbs = 0,
	.ldras = 0,
	.cpcs  = 1,
	.cpbs  = 0,
	.cpas  = 0,
	.lovrs = 0,
	.covfs = 0
};

//*******************************************************************
// Options for waveform generation.
static const tc_waveform_opt_t WAVEFORM_OPT[NCHANNELS] = {
{
	.channel  = ITimer0,                        	// Channel selection.

	.bswtrg   = TC_EVT_EFFECT_NOOP,                // Software trigger effect on TIOB.
	.beevt    = TC_EVT_EFFECT_NOOP,                // External event effect on TIOB.
	.bcpc     = TC_EVT_EFFECT_NOOP,                // RC compare effect on TIOB.
	.bcpb     = TC_EVT_EFFECT_NOOP,                // RB compare effect on TIOB.

	.aswtrg   = TC_EVT_EFFECT_NOOP,                // Software trigger effect on TIOA.
	.aeevt    = TC_EVT_EFFECT_NOOP,                // External event effect on TIOA.
	.acpc     = TC_EVT_EFFECT_NOOP,                // RC compare effect on TIOA: toggle.
	.acpa     = TC_EVT_EFFECT_NOOP,                // RA compare effect on TIOA: toggle (other possibilities are none, set and clear).

	.wavsel   = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,// Waveform selection: Up mode with automatic trigger(reset) on RC compare.
	.enetrg   = FALSE,                             // External event trigger enable.
	.eevt     = 0,                                 // External event selection.
	.eevtedg  = TC_SEL_NO_EDGE,                    // External event edge selection.
	.cpcdis   = FALSE,                             // Counter disable when RC compare.
	.cpcstop  = FALSE,                             // Counter clock stopped with RC compare.

	.burst    = FALSE,                             // Burst signal selection.
	.clki     = FALSE,                             // Clock inversion.
	.tcclks   = TC_CLOCK_SOURCE_TC4                // Internal source clock 2 - connected to PBA/16
},
{
	.channel  = ITimer1,							// Channel selection.

	.bswtrg   = TC_EVT_EFFECT_NOOP,                // Software trigger effect on TIOB.
	.beevt    = TC_EVT_EFFECT_NOOP,                // External event effect on TIOB.
	.bcpc     = TC_EVT_EFFECT_NOOP,                // RC compare effect on TIOB.
	.bcpb     = TC_EVT_EFFECT_NOOP,                // RB compare effect on TIOB.

	.aswtrg   = TC_EVT_EFFECT_NOOP,                // Software trigger effect on TIOA.
	.aeevt    = TC_EVT_EFFECT_NOOP,                // External event effect on TIOA.
	.acpc     = TC_EVT_EFFECT_NOOP,                // RC compare effect on TIOA: toggle.
	.acpa     = TC_EVT_EFFECT_NOOP,                // RA compare effect on TIOA: toggle (other possibilities are none, set and clear).

	.wavsel   = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,// Waveform selection: Up mode with automatic trigger(reset) on RC compare.
	.enetrg   = FALSE,                             // External event trigger enable.
	.eevt     = 0,                                 // External event selection.
	.eevtedg  = TC_SEL_NO_EDGE,                    // External event edge selection.
	.cpcdis   = FALSE,                             // Counter disable when RC compare.
	.cpcstop  = FALSE,                             // Counter clock stopped with RC compare.

	.burst    = FALSE,                             // Burst signal selection.
	.clki     = FALSE,                             // Clock inversion.
	.tcclks   = TC_CLOCK_SOURCE_TC4                // Internal source clock 2 - connected to PBA/16
},
{
	.channel  = ITimer1,							// Channel selection.

	.bswtrg   = TC_EVT_EFFECT_NOOP,                // Software trigger effect on TIOB.
	.beevt    = TC_EVT_EFFECT_NOOP,                // External event effect on TIOB.
	.bcpc     = TC_EVT_EFFECT_NOOP,                // RC compare effect on TIOB.
	.bcpb     = TC_EVT_EFFECT_NOOP,                // RB compare effect on TIOB.

	.aswtrg   = TC_EVT_EFFECT_NOOP,                // Software trigger effect on TIOA.
	.aeevt    = TC_EVT_EFFECT_NOOP,                // External event effect on TIOA.
	.acpc     = TC_EVT_EFFECT_NOOP,                // RC compare effect on TIOA: toggle.
	.acpa     = TC_EVT_EFFECT_NOOP,                // RA compare effect on TIOA: toggle (other possibilities are none, set and clear).

	.wavsel   = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,// Waveform selection: Up mode with automatic trigger(reset) on RC compare.
	.enetrg   = FALSE,                             // External event trigger enable.
	.eevt     = 0,                                 // External event selection.
	.eevtedg  = TC_SEL_NO_EDGE,                    // External event edge selection.
	.cpcdis   = FALSE,                             // Counter disable when RC compare.
	.cpcstop  = FALSE,                             // Counter clock stopped with RC compare.

	.burst    = FALSE,                             // Burst signal selection.
	.clki     = FALSE,                             // Clock inversion.
	.tcclks   = TC_CLOCK_SOURCE_TC4                // Internal source clock 2 - connected to PBA/16
}
};

//*******************************************************************
void
ITimer_Init(
	const ITimer		timer,
	const unsigned int	priority,
	const unsigned int	period_us,
	ITimer_Callback		timer_func
)
{
	volatile avr32_tc_t*	tc = &AVR32_TC;

	timer_callback_func[timer] = timer_func;

	// Register the RTC interrupt handler to the interrupt controller.
	switch (timer) {
	case ITimer0:
		INTC_register_interrupt(&tc_irq0, AVR32_TC_IRQ0, priority);
		break;
	case ITimer1:
		INTC_register_interrupt(&tc_irq1, AVR32_TC_IRQ1, priority);
		break;
	case ITimer2:
		INTC_register_interrupt(&tc_irq2, AVR32_TC_IRQ2, priority);
		break;
	}

	// Initialize the timer/counter.
	tc_init_waveform(tc, &WAVEFORM_OPT[timer]);

	// Set the compare triggers.
	// Remember TC counter is 16-bits, so counting second is not possible.
	// We configure it to count ms.
	// We want: (1/(F_PBA/4)) * RC = 1000 Hz => RC = (FPBA/4) / 1000 = 3000 to get an interrupt every 1ms
	tc_write_rc(tc, timer, ((F_PBA/16000) * period_us) / 1000);  // Set RC value.
	tc_configure_interrupts(tc, timer, &TC_INTERRUPT);

	tc_start(tc, timer);
}

