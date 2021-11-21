#ifndef Blockdevice_SDMMC_h_
#define Blockdevice_SDMMC_h_

#include <Filesystem/Blockdevice.h>
#include <stdint.h>

namespace Filesystem {

/** SD/MMC block device driver.

SPI functions will be used through Filesystem_SPI.h, which must be supplied by user with the following
functions/macros:
- FILESYSTEM_SDMMC_SPI_SELECT()
- FILESYSTEM_SDMMC_SPI_UNSELECT()
- FILESYSTEM_SDMMC_SPI_READ(uint16_t*)
- FILESYSTEM_SDMMC_SPI_WRITE(uint16_t)
*/

class Blockdevice_SDMMC : public Blockdevice {
public:
	/** Construct the block device and initialize SD/MMC card.
	SPI has to be initialized beforehand.

	Throws errors on exceptions.
	*/
	Blockdevice_SDMMC();
	virtual ~Blockdevice_SDMMC();

	virtual bool
	Read(
		const unsigned int	nr,
		void*				block
	);

	virtual bool
	Write(
		const unsigned int	nr,
		const void*			block
	);
private:
	// Is our card either SD_CARD or MMC_CARD?
	uint8_t		card_type_;
}; // class Blockdevice_SDMMC

} // namespace Filesystem

#endif /* Blockdevice_SDMMC_h_ */
