#include <stdio.h>

static unsigned int	F_PBA = 32000000;

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
 * \param[in]	BaudRate	Desired baud rate.
 * \param[out]	cd			Optimal Clock Divider
 * \param[out]	fp			Optimal Fractional Part
 * \param[out]	over		Optimal Overclock flag, 0=16x, 1=8x.
 * \return	UART baud rate error, promills (1/1000th).
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
		const int		cd0 = F_PBA / (8*(2-trial_over) * BaudRate) - 1;
		const int		cd1 = cd0 + 2;
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

	return best_error;
}

//*******************************************************************
static void
print_usart_settings(
	const unsigned int	baud_rate
	)
{
	unsigned int	cd;
	unsigned int	fp;
	unsigned int	over;
	unsigned int	error;

	error = FindOptimalUsartSettings(baud_rate, &cd, &fp, &over);
	const double	percentage = error * 100.0 / F_PBA;

	printf("%d => %d %d %d %3.1f%%\n", baud_rate, cd, fp, over, percentage);
}

//*******************************************************************
int
main(
	int	argc,
	char**	argv)
{
	printf("F_PBA=%d Hz\n", F_PBA);
	print_usart_settings(19200);
	print_usart_settings(250000);
	print_usart_settings(57600);
	F_PBA = 12000000;
	printf("F_PBA=%d Hz\n", F_PBA);
	print_usart_settings(19200);
	print_usart_settings(250000);
	print_usart_settings(57600);
	return 0;
}
