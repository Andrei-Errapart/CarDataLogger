/**
vim: ts=4
vim: shiftwidth=4
*/
#ifndef IClock_h_
#define IClock_h_

#ifdef __cplusplus
extern "C" {
#endif

extern void delay_ms(const unsigned short time_ms);

/** Start PLL0, enable a generic clock with PLL0 output then switch main clock to PLL0 output.
   All calculations in this function suppose that the Osc0 frequency is 12MHz.

This also outputs clock at PB19, pin 51 (red led on EVK1100).
   */
extern void PLL0_Start(void);

/** CPU tick count since startup. */
#define GetTSC	Get_sys_count

/** Time since system startup, milliseconds.
This uses F_CPU macro to determine core speed. */
extern unsigned long GetTickCount(void);

#ifdef __cplusplus
}
#endif

#endif /* IClock_h_ */
