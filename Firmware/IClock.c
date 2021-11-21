/**
vim: ts=4
vim: shiftwidth=4
*/

#include "project.h"
#include "IClock.h"

//*******************************************************************
/*! \brief Waits during at least the specified delay before returning.
 *
 * \param ck Number of HSB clock cycles to wait.
 */
static void delay_ck(const unsigned long ck)
{
	// Use the CPU cycle counter (CPU and HSB clocks are the same).
	const unsigned long delay_start_cycle = Get_system_register(AVR32_COUNT);
	const unsigned long delay_end_cycle = delay_start_cycle + ck;

	// To be safer, the end of wait is based on an inequality test, so CPU cycle
	// counter wrap around is checked.
	if (delay_start_cycle <= delay_end_cycle) {
		while ((unsigned long)Get_system_register(AVR32_COUNT) < delay_end_cycle);
	} else {
		while ((unsigned long)Get_system_register(AVR32_COUNT) > delay_end_cycle);
	}
}


//*******************************************************************
void delay_ms(const unsigned short time_ms)
{
	delay_ck( (unsigned long)time_ms * (F_CPU / 1000) );
}

#  define CPUCLOCK_GCLK_ID             0
#  define CPUCLOCK_GCLK_PIN            AVR32_PM_GCLK_0_1_PIN
#  define CPUCLOCK_GCLK_FUNCTION       AVR32_PM_GCLK_0_1_FUNCTION
// Note that gclk0_1 is pin 51 pb19 on AT32UC3A0512 QFP144.

//*******************************************************************
void PLL0_Start(void)
{
	volatile avr32_pm_t* pm = &AVR32_PM;
	pm_switch_to_osc0(pm, FOSC0, OSC0_STARTUP);  // Switch main clock to Osc0.

	/* Setup PLL0 on Osc0, mul=3 ,no divisor, lockcount=16, ie. 12Mhzx8 = 96MHz output */
	pm_pll_setup(pm,
			   0,   // use PLL0
			   (2*F_CPU_MULTIPLIER-1),   // MUL=9 in the formula
			   F_CPU_DIVIDER,   // DIV=1 in the formula
			   0,   // Sel Osc0/PLL0 or Osc1/PLL1
			   16); // lockcount in main clock for the PLL wait lock

	/*
	This function will set a PLL option.
	*pm Base address of the Power Manager (i.e. &AVR32_PM)
	pll PLL number 0
	pll_freq Set to 1 for VCO frequency range 80-180MHz, set to 0 for VCO frequency range 160-240Mhz.
	pll_div2 Divide the PLL output frequency by 2 (this settings does not change the FVCO value)
	pll_wbwdisable 1 Disable the Wide-Bandith Mode (Wide-Bandwith mode allow a faster startup time and out-of-lock time). 0 to enable the Wide-Bandith Mode.
	*/
	/* PLL output VCO frequency is 96MHz. We divide it by 2 with the pll_div2=1. This enable to get later main clock to 48MHz */
	pm_pll_set_option(pm, 0, (FOSC0*F_CPU_MULTIPLIER)>170000000 ? 0 : 1, 0, 0);

	/* Enable PLL0 */
	pm_pll_enable(pm,0);

	/* Wait for PLL0 locked */
	pm_wait_for_pll0_locked(pm) ;

	/* Setup generic clock on PLL0, with Osc0/PLL0, no divisor */
	pm_gc_setup(pm,
			  CPUCLOCK_GCLK_ID,
			  1,  // Use Osc (=0) or PLL (=1), here PLL
			  0,  // Sel Osc0/PLL0 or Osc1/PLL1
			  0,  // disable divisor
			  0); // no divisor

	/* Enable Generic clock */
	pm_gc_enable(pm, CPUCLOCK_GCLK_ID);

	/* Set the GCLOCK function to the GPIO pin */
	gpio_enable_module_pin(CPUCLOCK_GCLK_PIN, CPUCLOCK_GCLK_FUNCTION);


	/* Divide PBA clock by 2 from main clock (PBA clock = 48MHz/2 = 24MHz).
	 Pheripheral Bus A clock divisor enable = 1
	 Pheripheral Bus A select = 0
	 Pheripheral Bus B clock divisor enable = 0
	 Pheripheral Bus B select = 0
	 High Speed Bus clock divisor enable = 0
	 High Speed Bus select = 0
	*/
	pm_cksel(pm, 1, 0, 0, 0, 0, 0);

	// Set one wait-state (WS) for flash controller. 0 WS access is up to 30MHz for HSB/CPU clock.
	// As we want to have 48MHz on HSB/CPU clock, we need to set 1 WS on flash controller.
	flashc_set_wait_state(1);

	pm_switch_to_clock(pm, AVR32_PM_MCSEL_PLL0); /* Switch main clock to 48MHz */
}

//*******************************************************************
unsigned long
GetTickCount(void)
{
	return Get_sys_count() / (F_CPU / 1000);
}

