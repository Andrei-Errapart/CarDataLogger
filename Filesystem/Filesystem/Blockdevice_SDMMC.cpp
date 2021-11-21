/**
vim: ts=4
vim: shiftwidth=4
*/
#include <string.h>	// memset.h

#include <Filesystem/Blockdevice_SDMMC.h>
#include <Filesystem_Config.h>
#include <Filesystem/Error.h>


#if defined(_MSC_VER)
//*******************************************************************
static void
FILESYSTEM_SDMMC_SPI_SELECT()
{
}

//*******************************************************************
static void
FILESYSTEM_SDMMC_SPI_UNSELECT()
{
}

//*******************************************************************
static void
FILESYSTEM_SDMMC_SPI_READ(
	uint16_t*	data
)
{
}

//*******************************************************************
static void
FILESYSTEM_SDMMC_SPI_WRITE(
	const uint16_t	data
)
{
}

#define	delay_ms(x)	do { } while (0)
#else
// this is slightly a hack. don't worry :)
#include "IClock.h"
#endif

// Card identification
#define MMC_CARD                          0
#define SD_CARD                           1

// MMC commands (taken from sandisk MMC reference)
#define MMC_GO_IDLE_STATE                 0    ///< initialize card to SPI-type access
#define MMC_SEND_OP_COND                  1    ///< set card operational mode
#define MMC_CMD2                          2               ///< illegal in SPI mode !
#define MMC_SEND_CSD                      9    ///< get card's CSD
#define MMC_SEND_CID                      10    ///< get card's CID
#define MMC_SEND_STATUS                   13
#define MMC_SET_BLOCKLEN                  16    ///< Set number of bytes to transfer per block
#define MMC_READ_SINGLE_BLOCK             17    ///< read a block
#define MMC_WRITE_BLOCK                   24    ///< write a block
#define MMC_PROGRAM_CSD                   27
#define MMC_SET_WRITE_PROT                28
#define MMC_CLR_WRITE_PROT                29
#define MMC_SEND_WRITE_PROT               30
#define SD_TAG_WR_ERASE_GROUP_START       32
#define SD_TAG_WR_ERASE_GROUP_END         33
#define MMC_TAG_SECTOR_START              32
#define MMC_TAG_SECTOR_END                33
#define MMC_UNTAG_SECTOR                  34
#define MMC_TAG_ERASE_GROUP_START         35    ///< Sets beginning of erase group (mass erase)
#define MMC_TAG_ERASE_GROUP_END           36    ///< Sets end of erase group (mass erase)
#define MMC_UNTAG_ERASE_GROUP             37    ///< Untag (unset) erase group (mass erase)
#define MMC_ERASE                         38    ///< Perform block/mass erase
#define SD_SEND_OP_COND_ACMD              41              ///< Same as MMC_SEND_OP_COND but specific to SD (must be preceeded by CMD55)
#define MMC_LOCK_UNLOCK                   42              ///< To start a lock/unlock/pwd operation
#define SD_APP_CMD55                      55              ///< Use before any specific command (type ACMD)
#define MMC_CRC_ON_OFF                    59    ///< Turns CRC check on/off
// R1 Response bit-defines
#define MMC_R1_BUSY                       0x80  ///< R1 response: bit indicates card is busy
#define MMC_R1_PARAMETER                  0x40
#define MMC_R1_ADDRESS                    0x20
#define MMC_R1_ERASE_SEQ                  0x10
#define MMC_R1_COM_CRC                    0x08
#define MMC_R1_ILLEGAL_COM                0x04
#define MMC_R1_ERASE_RESET                0x02
#define MMC_R1_IDLE_STATE                 0x01
// Data Start tokens
#define MMC_STARTBLOCK_READ               0xFE  ///< when received from card, indicates that a block of data will follow
#define MMC_STARTBLOCK_WRITE              0xFE  ///< when sent to card, indicates that a block of data will follow
#define MMC_STARTBLOCK_MWRITE             0xFC
// Data Stop tokens
#define MMC_STOPTRAN_WRITE                0xFD
// Data Error Token values
#define MMC_DE_MASK                       0x1F
#define MMC_DE_ERROR                      0x01
#define MMC_DE_CC_ERROR                   0x02
#define MMC_DE_ECC_FAIL                   0x04
#define MMC_DE_OUT_OF_RANGE               0x04
#define MMC_DE_CARD_LOCKED                0x04
// Data Response Token values
#define MMC_DR_MASK                       0x1F
#define MMC_DR_ACCEPT                     0x05
#define MMC_DR_REJECT_CRC                 0x0B
#define MMC_DR_REJECT_WRITE_ERROR         0x0D

/**
 * MMC/SD card SPI mode commands
 **/
#define CMD0 0x40	// software reset
#define CMD1 0x41	// brings card out of idle state
#define CMD2 0x42	// not used in SPI mode
#define CMD3 0x43	// not used in SPI mode
#define CMD4 0x44	// not used in SPI mode
#define CMD5 0x45	// Reserved
#define CMD6 0x46	// Reserved
#define CMD7 0x47	// not used in SPI mode
#define CMD8 0x48	// Reserved
#define CMD9 0x49	// ask card to send card speficic data (CSD)
#define CMD10 0x4A	// ask card to send card identification (CID)
#define CMD11 0x4B	// not used in SPI mode
#define CMD12 0x4C	// stop transmission on multiple block read
#define CMD13 0x4D	// ask the card to send it's status register
#define CMD14 0x4E	// Reserved
#define CMD15 0x4F	// not used in SPI mode
#define CMD16 0x50	// sets the block length used by the memory card
#define CMD17 0x51	// read single block
#define CMD18 0x52	// read multiple block
#define CMD19 0x53	// Reserved
#define CMD20 0x54	// not used in SPI mode
#define CMD21 0x55	// Reserved
#define CMD22 0x56	// Reserved
#define CMD23 0x57	// Reserved
#define CMD24 0x58	// writes a single block
#define CMD25 0x59	// writes multiple blocks
#define CMD26 0x5A	// not used in SPI mode
#define CMD27 0x5B	// change the bits in CSD
#define CMD28 0x5C	// sets the write protection bit
#define CMD29 0x5D	// clears the write protection bit
#define CMD30 0x5E	// checks the write protection bit
#define CMD31 0x5F	// Reserved
#define CMD32 0x60	// Sets the address of the first sector of the erase group
#define CMD33 0x61	// Sets the address of the last sector of the erase group
#define CMD34 0x62	// removes a sector from the selected group
#define CMD35 0x63	// Sets the address of the first group
#define CMD36 0x64	// Sets the address of the last erase group
#define CMD37 0x65	// removes a group from the selected section
#define CMD38 0x66	// erase all selected groups
#define CMD39 0x67	// not used in SPI mode
#define CMD40 0x68	// not used in SPI mode
#define CMD41 0x69	// Reserved
#define CMD42 0x6A	// locks a block
// CMD43 ... CMD57 are Reserved
#define CMD58 0x7A	// reads the OCR register
#define CMD59 0x7B	// turns CRC off
// CMD60 ... CMD63 are not used in SPI mode


namespace Filesystem {

//*******************************************************************
namespace {
class SpiAutoselect {
public:
	SpiAutoselect() {
		FILESYSTEM_SDMMC_SPI_SELECT();
	}
	~SpiAutoselect() {
		FILESYSTEM_SDMMC_SPI_UNSELECT();
	}
}; // class Autoselect
} // namespace anonymous

//*******************************************************************
static unsigned short
send_and_read(
	const unsigned char data_to_send
)
{
	unsigned short data_read;
	FILESYSTEM_SDMMC_SPI_WRITE(data_to_send);
	FILESYSTEM_SDMMC_SPI_READ(&data_read);
	return data_read;
}

//*******************************************************************
//!
//! @brief This function sends a command WITH DATA STATE to the SD/MMC and waits for R1 response
//!        The memory /CS signal is not affected so this function can be used to send a commande during a large transmission
//!
//! @warning Code:?? bytes (function code length)
//!
//! @param  command   command to send (see sd_mmc.h for command list)
//!         arg       argument of the command
//!
//! @return U8
//!         R1 response (R1 == 0xFF time out error)
//!/
static uint8_t
sd_mmc_command(const uint8_t command, const uint32_t arg)
{
	uint8_t	retry;
	uint8_t	r1;

	FILESYSTEM_SDMMC_SPI_WRITE(0xFF);            // write dummy byte
	FILESYSTEM_SDMMC_SPI_WRITE(command | 0x40);  // send command
	FILESYSTEM_SDMMC_SPI_WRITE(arg>>24);         // send parameter
	FILESYSTEM_SDMMC_SPI_WRITE(arg>>16);
	FILESYSTEM_SDMMC_SPI_WRITE(arg>>8 );
	FILESYSTEM_SDMMC_SPI_WRITE(arg    );
	FILESYSTEM_SDMMC_SPI_WRITE(0x95);            // correct CRC for first command in SPI (CMD0)
								  // after, the CRC is ignored
	// end command
	// wait for response
	// if more than 8 retries, card has timed-out and return the received 0xFF
	retry = 0;
	r1    = 0xFF;
	while ((r1 = send_and_read(0xFF)) == 0xFF) {
		retry++;
		if (retry > 10) {
			break;
		}
	}
	return r1;
}

//*******************************************************************
//!
//! @brief Waits until the SD/MMC is not busy.
//!
//! @return bit
//!          OK when card is not busy
//!/
static bool
wait_not_busy(void)
{
	uint16_t		retry;
	uint8_t			r1;

	retry = 0;
	while ((r1 = send_and_read(0xFF)) != 0xFF) {
		retry++;
		if (retry == 50000) {
			filesystem_dprintf(("sd_mmc: wait_not_busy timeout.\n"));
			return false;
		}
	}
	return true;
}

//*******************************************************************
/** Sends the command to the the memory card with the 4 arguments and the CRC
 *
 *	@param command		the command
 *	@param argX			the arguments
 *	@param CRC			cyclic redundancy check
 **/
void
sendCommand(
	unsigned char command,
	unsigned char argA,
	unsigned char argB,
	unsigned char argC,
	unsigned char argD,
	unsigned char CRC
)
{	
	send_and_read(command);
	send_and_read(argA);
	send_and_read(argB);
	send_and_read(argC);
	send_and_read(argD);
	send_and_read(CRC);
}

//*******************************************************************
/**
 *	waits for the memory card to response
 *
 *	@param expRes		expected response
 *
 *	@return true		card responded within time limit
 *	@return false		card did not response
 **/
static bool
cardResponse(unsigned char expRes)
{
	unsigned char actRes	= 0xFF;
	int count				= 1000;
	do {
		actRes = send_and_read(0xff);		
		count--;
	} while ((actRes != expRes) && (count > 0));
	return count>0;
}

//*******************************************************************
/**
 *	waits for the memory card to response
 *
 *	@param expRes		expected response
 *
 *	@return true		card responded within time limit
 *	@return false		card did not response
 **/
static bool
cardResponse(
	unsigned char	expRes,
	unsigned char&	response
	)
{
	response				= 0xFF;
	int count				= 1000;
	do {
		response = send_and_read(0xff);		
		// filesystem_dprintf(("0x%02X ", response));
		count--;
	} while ((response != expRes) && (count > 0));
	return count>0;
}

#define UNSTUFF_BITS(resp,start,size)                                   \
        ({                                                              \
                const int __size = size;                                \
                const uint8_t __mask = (__size < 8 ? 1 << __size : 0) - 1; \
                const int __off = 15 - ((start) / 8);                   \
                const int __shft = (start) & 7;                         \
                uint8_t __res;                                          \
                                                                        \
                __res = resp[__off] >> __shft;                          \
                if (__size + __shft > 8)                                \
                        __res |= resp[__off-1] << ((8 - __shft) % 8);   \
                __res & __mask;                                         \
        })
//*******************************************************************
static void
print_csd_field(
	const char*			name,
	const uint8_t*		csd,
	const unsigned int	start,
	const unsigned int	size
)
{
	const uint8_t	v = UNSTUFF_BITS(csd, start, size);
	filesystem_dprintf(("%s : %02X\n", name, v));
}

//*******************************************************************
//! @brief Reads the CSD (Card Specific Data) of the memory card
//!
//! @param  buffer to fill
//!
//!/
void
sd_mmc_get_csd()
{
	uint8_t			buffer[16];
	uint8_t			r1;
	unsigned int	retry;
	unsigned int	i;
	unsigned short	data_read;

	SpiAutoselect	sa;

	// wait for MMC not busy
	if (!wait_not_busy()) {
		throw Error("MemCard busy, no CSD.");
	}


	// issue command
	r1 = sd_mmc_command(MMC_SEND_CSD, 0);
	// check for valid response
	if (r1 != 0x00) {
		throw Error("MemCard CSD fail 1.");
	}
	// wait for block start
	retry = 0;

	while ((r1 = send_and_read(0xFF)) != MMC_STARTBLOCK_READ) {
		if (retry > 8) {
			throw Error("MemCard CSD timeout.");
		}
		retry++;
	}

	for (i= 0; i<16; ++i) {
		FILESYSTEM_SDMMC_SPI_WRITE(0xFF);
		FILESYSTEM_SDMMC_SPI_READ(&data_read);
		buffer[i] = data_read;
	}
	FILESYSTEM_SDMMC_SPI_WRITE(0xFF);   // load CRC (not used)
	FILESYSTEM_SDMMC_SPI_WRITE(0xFF);
	FILESYSTEM_SDMMC_SPI_WRITE(0xFF);   // give clock again to end transaction

	filesystem_dprintf(("CSD:"));
	for (i=0; i<16; ++i) {
		filesystem_dprintf((" %02X", buffer[i]));
	}
	filesystem_dprintf(("\n"));

	print_csd_field("csd struct", buffer, 126, 2);
	const unsigned int cap_e = UNSTUFF_BITS(buffer, 47, 3);
	const unsigned int cap_m = UNSTUFF_BITS(buffer, 62, 12);
	const unsigned int capacity     = (1 + cap_m) << (cap_e + 2);
	filesystem_dprintf(("Capacity: %d\n", capacity));
	print_csd_field("read_blkbits  ", buffer, 80, 4);
	print_csd_field("read_partial  ", buffer, 79, 1);
	print_csd_field("write_misalign", buffer, 78, 1);
	print_csd_field("read_misalign ", buffer, 77, 1);
	print_csd_field("r2w_factor    ", buffer, 26, 3);
	print_csd_field("write_blkbits ", buffer, 22, 4);
	print_csd_field("write_partial ", buffer, 21, 1);
}

//*******************************************************************
/**
 *	initializes the memory card by bring it out of idle state and 
 *	sets it up to use SPI mode
 **/
void
memCardInit(
	uint8_t&		card_type
)
{
	uint32_t		retry = 0;
	uint8_t			r1 = 0;

	// Atmel EVK1100 specific :(
	// Synchronize the memory card. For some unknown reason :( the following is required:
	// 1. CS line of SD/MMC is NOT selected!
	// 2. Atmel SPI interface requires at least one CS line to be selected in order to output any data.
	// The LCD display loses. I am very sorry. Hopefully she is not angry at me.
	spi_selectChip(DIP204_SPI, DIP204_SPI_CS);
	for (unsigned int i=0; i<10; ++i) {
		r1 = send_and_read(0xFF);
	}
	spi_unselectChip(DIP204_SPI, DIP204_SPI_CS);
	filesystem_dprintf(("sdmmc: reset1 r=0x%02X\n", r1));

	SpiAutoselect	sa;

	// RESET THE MEMORY CARD
	card_type = MMC_CARD;
	retry = 0;
	do {
		// reset card and go to SPI mode
		r1 = sd_mmc_command(MMC_GO_IDLE_STATE, 0);
		send_and_read(0xFF);            // write dummy byte

		// do retry counter
		retry++;
		if (retry > 100) {
			throw Error("MemCard Missing.");
		}
		if (((retry + 1) % 30) == 0) {
			delay_ms(1);
		}
	} while(r1 != 0x01);   // check memory enters idle_state

	// IDENTIFICATION OF THE CARD TYPE (SD or MMC)
	// Both cards will accept CMD55 command but only the SD card will respond to ACMD41
	r1 = sd_mmc_command(SD_APP_CMD55,0);
	send_and_read(0xFF);  // write dummy byte

	r1 = sd_mmc_command(SD_SEND_OP_COND_ACMD, 0);
	send_and_read(0xFF);  // write dummy byte

	if ((r1&0xFE) == 0) {   // ignore "in_idle_state" flag bit
		card_type = SD_CARD;    // card has accepted the command, this is a SD card
		filesystem_dprintf(("sd_mmc: detected SD card.\n"));
	} else {
		card_type = MMC_CARD;   // card has not responded, this is a MMC card
		filesystem_dprintf(("sd_mmc: querying MMC card.\n"));
		// reset card again
		retry = 0;
		do {
			// reset card again
			r1 = sd_mmc_command(MMC_GO_IDLE_STATE, 0);
			send_and_read(0xFF);            // write dummy byte
			// do retry counter
			retry++;
			if (retry > 100) {
				throw Error("MemCard reset timeout.\n");
			}
		} while(r1 != 0x01);   // check memory enters idle_state
	}

	// CONTINUE INTERNAL INITIALIZATION OF THE CARD
	// Continue sending CMD1 while memory card is in idle state
	retry = 0;
	do {
		// initializing card for operation
		r1 = sd_mmc_command(CMD1, 0);
		send_and_read(0xFF);            // write dummy byte
		// do retry counter
		retry++;
		if (retry == 150000) {    // measured approx. 500 on several cards
			throw Error("MemCard init timeout.\n");
		}
	} while (r1);

	// DISABLE CRC TO SIMPLIFY AND SPEED UP COMMUNICATIONS
	r1 = sd_mmc_command(MMC_CRC_ON_OFF, 0);  // disable CRC (should be already initialized on SPI init)
	send_and_read(0xFF);            // write dummy byte

	// SET BLOCK LENGTH TO 512 BYTES
	r1 = sd_mmc_command(MMC_SET_BLOCKLEN, Blockdevice::BLOCK_SIZE);
	send_and_read(0xFF);            // write dummy byte
	if (r1 != 0x00) {
		throw Error("MemCard unsupported.");
	}

	filesystem_dprintf(("sd_mmc: memCardInit OK.\n"));
}

//*******************************************************************
/**
 *	Initialize the block length in the memory card to be 512 bytes
 *	
 *	return true			block length sucessfully set
 *	return false		block length not sucessfully set
 **/
void
setBlockLength512()
{
	bool	r = false;
	{
		SpiAutoselect	sa;

		wait_not_busy();
		sendCommand(CMD16,0,0,2,0,0xFF);
		r = cardResponse(0x00);
		send_and_read(0xFF);
	}

	if (!r) {
		throw Error("MemCard SetBlockLength failed.");
	}
}

//*******************************************************************
Blockdevice_SDMMC::Blockdevice_SDMMC()
:	card_type_(0)
{
	memCardInit(card_type_);
	setBlockLength512();
	sd_mmc_get_csd();
}

//*******************************************************************
Blockdevice_SDMMC::~Blockdevice_SDMMC()
{
}

//*******************************************************************
bool
Blockdevice_SDMMC::Read(
	const unsigned int	nr,
	void*				block
)
{
	unsigned int	block256 = nr * 2;
	bool			ok = false;
	
	filesystem_dprintf(("Blockdevice_SDMMC::Read 0x%04lX\n", nr));

	SpiAutoselect	sa;
	wait_not_busy();

	sendCommand(CMD17,(block256>>16)&0xFF,(block256>>8)&0xFF,block256&0xFF,0,0xFF);
	if (cardResponse(0x00))	{
		unsigned char	real_response = 0xFF;
		if (cardResponse(0xFE, real_response)) {
			char*	buf = reinterpret_cast<char*>(block);
			for (unsigned int count = 0; count < BLOCK_SIZE; count++) {
				buf[count] = send_and_read(0xff);
			}
			send_and_read(0xff);
			send_and_read(0xff);
			ok = true;
		} else {
			throw Error("MemCard Read Fail, 0x%02X, block %04X.", real_response, nr);
		}
	} else {
		throw Error("MemCard Read Fail 2, block %04X.", nr);
	}
	send_and_read(0xff);

	return ok;
}

//*******************************************************************
bool
Blockdevice_SDMMC::Write(
	const unsigned int	nr,
	const void*			block
)
{
	filesystem_dprintf(("Blockdevice_SDMMC::Write  0x%04lX\n", nr));

	unsigned int	block256 = nr * 2;
	SpiAutoselect	sa;

#if (1)
	const unsigned int	max_retries = 3;
	for (unsigned int retry=0; retry<max_retries; ++retry) {
		wait_not_busy();
		// issue command
		sendCommand(CMD24, (block256>>16)&0xFF, (block256>>8)&0xFF, block256&0xFF, 0, 0xFF);
		uint8_t r1 = 0xFF;
		{
			// end command
			// wait for response
			// if more than 8 retries, card has timed-out and return the received 0xFF
			for (unsigned int i=0; i<=20; ++i) {
				r1 = send_and_read(0xFF);
				if (r1 != 0xFF) {
					break;
				}
			}
		}

		// check for valid response
		if (r1 != 0x00) {
			delay_ms(1);
			if (retry+1 == max_retries) {
				throw Error("MemCard Not Responding, r1=0x%02X, block %04X.", r1, nr);
			}
		}

		// send dummy
		send_and_read(0xFF);		// give clock again to end transaction

		// send data start token
		send_and_read(MMC_STARTBLOCK_WRITE);
		// write data
		const char*	buf = reinterpret_cast<const char*>(block);
		for (unsigned int i=0; i<BLOCK_SIZE; ++i) {
			send_and_read(buf[i]);
		}

		send_and_read(0xFF);		// send CRC (field required but value ignored)
		send_and_read(0xFF);

		// read data response token
		r1 = send_and_read(0xFF);
		send_and_read(0xFF);		// send dummy bytes
		send_and_read(0xFF);
		if ((r1&MMC_DR_MASK) == MMC_DR_ACCEPT) {
			// Without wait_not_busy the memory card will listen to the traffic
			// with other SPI devices and screw up our write.
			wait_not_busy();
			return true;
		} else {
			delay_ms(1);
			if (retry+1 == max_retries) {
				throw Error("MemCard Invalid Response 0x%02X, block %04X.", r1, nr);
			}
		}
	}
#endif

#if (0)
	const unsigned int	max_retries = 3;
	for (unsigned int retry=0; retry<max_retries; ++retry) {
		wait_not_busy();
		sendCommand(CMD24,(block256>>16)&0xFF,(block256>>8)&0xFF,block256&0xFF,0,0xFF);
		if (cardResponse(0x00)) {
			const char*	buf = reinterpret_cast<const char*>(block);

			send_and_read(0xFF);
			send_and_read(0xFE);
			for (unsigned int count = 0; count < BLOCK_SIZE; ++count) {
				send_and_read(buf[count]);
			}
			send_and_read(0xFF);
			send_and_read(0xFF);

			// check write state.
			unsigned int count = 0;
			bool			ok = true;
			while(count <= 64)
			{
				const char inRes = send_and_read(0xFF) & 0x1F;
				if (inRes == 0x05) {
					break;
				} else if (inRes == 0x0B || inRes==0x0D) {
					// bad :(
					ok = false;
					break;
				}
			}
			if (ok) {
				break;
			} else if (retry+1>=max_retries) {
				throw Error("MemCard write error, block 0x%04X", nr);
			} else {
				// and then continue.
				delay_ms(1);
			}
		}
	}
	wait_not_busy();
	return true;
#endif

	return false;
}

} // namespace Filesystem
