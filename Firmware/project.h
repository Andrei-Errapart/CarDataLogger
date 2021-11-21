#ifndef _PROJECT_H
#define _PROJECT_H

#include "board.h"
#include "compiler.h"
#include "dip204.h"
#include "intc.h"
#include "gpio.h"
#include "pm.h"
#include "count.h"
#include "spi.h"
#include "usart.h"
#include "tc.h"
#include "flashc.h"
#include "sdramc.h"
#include <avr32/io.h>

// OSC0 runs at 12 MHz.
// CPU clock runs at 64 MHz.
// Periferial bus access is 32 MHz.
#define	F_CPU_MULTIPLIER	16
#define F_CPU_DIVIDER		3
#define F_CPU		(FOSC0 * F_CPU_MULTIPLIER / F_CPU_DIVIDER)
#define F_PBA    	(F_CPU / 2)

#endif  // _PROJECT_H
