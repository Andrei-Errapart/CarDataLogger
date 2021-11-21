/**
vim: ts=4
vim: shiftwidth=4
*/

#ifndef pmalloc_h_
#define	pmalloc_h_


#ifdef __cplusplus
extern "C" {
#endif

/** \file Permamently allocate pieces off the external SDRAM storage.
Allocated memory alignment is 4 bytes.
 */

typedef enum {
	/** Skip external memory check. */
	PMALLOC_INIT_SKIPCHECK,
	/** Check external memory throughly. */
	PMALLOC_INIT_CHECK
} PMALLOC_INIT;

/** Permamently allocate given size of memory. */
extern void*
pmalloc(	const unsigned int	size);

/** Number of bytes available. */
extern unsigned int
pavailable(void);

/** Initialize external SDRAM. */
extern void
pmalloc_init(
	const PMALLOC_INIT	flags
);

#ifdef __cplusplus
}
#endif

#endif /* pmalloc_h_ */

