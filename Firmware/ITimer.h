/*
vim: ts=4
vim: shiftwidth=4
*/
#ifndef ITimer_h_
#define ITimer_h_

#if defined(__cplusplus)
extern "C" {
#endif

/** Timer callback. Argument is a character received. */
typedef void (*ITimer_Callback)(void);

/** Timer index. */
typedef enum {
	/** Timer 0. */
	ITimer0	= 0,
	/** Timer 1. */
	ITimer1	= 1,
	/** Timer 2, */
	ITimer2	= 2,
} ITimer;

/** Initialize periodic timer with the given period, microseconds.
 * Note 1: Only one timer at a time.
 * Note 2: Since timer is only 16 bits, the maximum time is limited to 65536 / (F_PBA/divider) * 1000_000 microseconds.
 *         With 32MHz F_PBA, 16 divider, it is limited to 34952 microseconds.
 *
 * \param[in]	timer		Timer number.
 * \param[in]	priority	Timer interrupt priority - INT0 (highest) to INT3 (lowest).
 * \param[in]	period_us	Timer period, microseconds.
 * \param[in]	timer_func	Timer callback function.
 */
extern void ITimer_Init(
	const ITimer		timer,
	const unsigned int	priority,
	const unsigned int	period_us,
	ITimer_Callback		timer_func
);

#if defined(__cplusplus)
}
#endif

#endif /* ITimer_h_ */

