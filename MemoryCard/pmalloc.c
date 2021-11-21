/**
vim: ts=4
vim: shiftwidth=4
*/

#include <stdint.h>
#include <stdio.h>

#include "project.h"
#include "pmalloc.h"
#include "Display.h"
#include <print_funcs.h>

/** Pointer to the SDRAM memory block. */
static uint32_t*			sdram = SDRAM;
/** Number of words in the SDRAM memory memory. */
static const unsigned int	sdram_size = SDRAM_SIZE >> 2;
/** First free word index into SDRAM memory block. */
static unsigned int			free_index = 0;

//*******************************************************************
void*
pmalloc(
	const unsigned int	size
)
{
	const unsigned int	size4 = (size + 3) / 4;
	if (free_index + size4 <= sdram_size) {
		uint32_t*	r = sdram + size4;
		free_index += size4;
		return r;
	} else {
		// OUT OF MEMORY.
		Display_FatalError("pmalloc: OUT OF MEMORY");
		return 0;
	}
}

//*******************************************************************
unsigned int
pavailable(void)
{
	return 4 * (sdram_size - free_index);
}

//*******************************************************************
#define sdram_check(sdram,i, v) do {	\
	sdram[i]= (v);						\
	if (sdram[i] != (v)) {				\
		goto sdram_failure;				\
	}									\
} while (0)

//*******************************************************************
void
pmalloc_init(
	const PMALLOC_INIT	flags
)
{
	volatile uint32_t*	xsdram = SDRAM;
	const unsigned int	progress_inc = 1024*1024 / 4;
	unsigned int		progress_i = 0;
	unsigned int		progress_mb = 0;
	unsigned int		i;
	char				xbuf[80];

	print_dbg("pmalloc: calling sdramc_init...\n");
	// Initialize the external SDRAM chip.
	sdramc_init(F_CPU);
	print_dbg("pmalloc: sdramc_init SUCCESS.n");

	// Display SDRAM size.
	dip204_set_cursor_position(1,1);
	sprintf(xbuf, "CPU:%dMHz MEM:%dMB", F_CPU/1000000, sdram_size/progress_inc);
	dip204_write_string(xbuf);

	if (flags == PMALLOC_INIT_CHECK) {
		for (i=0; i<sdram_size; ++i) {
			sdram_check(xsdram,i,0);
			sdram_check(xsdram,i,0xFFFFFFFF);
			sdram_check(xsdram,i,0xAAAAAAAA);
			sdram_check(xsdram,i,0x55555555);
			if (i == progress_i) {
				dip204_set_cursor_position(1,2);
				sprintf(xbuf, "Verifying:%2dMB", progress_mb);
				dip204_write_string(xbuf);
				progress_i += progress_inc;
				progress_mb += 1;
			}
			continue;
	sdram_failure:
			dip204_set_cursor_position(1,2);
			dip204_write_string("MEMORY ERROR!");
			dip204_set_cursor_position(1,3);
			sprintf(xbuf, "Addr: 0x%08X", i);
			dip204_write_string(xbuf);
			for (;;);
		}
	}
}

