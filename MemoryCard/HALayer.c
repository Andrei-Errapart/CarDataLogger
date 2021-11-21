/**
vim: ts=4
vim: shiftwidth=4
*/

/**
 *	HALayer.c:	implementation of the HALayer.
 *	class HALayer:	defines the hardware specific portions
 *					of the MMC/SD card FAT library.  This file
 *					needs to be adapted for the specific microcontroller
 *					of choice
 *
 *	Author:	Ivan Sham
 *	Date: June 26, 2004
 *	Version: 2.0
 *	Note: Developed for William Hue and Pete Rizun
 *
 *****************************************************************
 *  Change Log
 *----------------------------------------------------------------
 *  Date    |   Author          | Reason for change
 *----------------------------------------------------------------
 * Aug31/04     William Hue       Removed convertCharToLong().
 *                                Changed char types to
 *                                unsigned char.
 *
 * Jul18/04     Alex Jiang        Added void parameter to
 *                                functions with no parameters.
 *                                Made delay a static function.
 *
 * Jan03/05     William Hue       Clean-up for
 *                                Circuit Cellar article.
 **/

#include <stdio.h>
#include "HALayer.h"
#include "project.h"
#include <stdint.h>

#if (FATLOG)
#include "print_funcs.h"
#else
#define	print_dbg(x)	do { } while (0)
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



extern unsigned long sectorZero;

/**
 *	case insensitive compare of the 2 inputs
 *	returns true if and only if c1 = c2 or if
 *	c1 and c2 are the same character with different
 *	capitialization
 *
 *	@param	c1			input 1
 *	@param	c2			input 2
 **/
bool charEquals(const char c1, const char c2)
{
	if((((c1 >= 'A')&&(c1 <= 'Z'))||((c1 >= 'a')&&(c1 <= 'z')))&&
		(((c2 >= 'A')&&(c2 <= 'Z'))||((c2 >= 'a')&&(c2 <= 'z'))))
	{
		if((c1 | 0x20) == (c2 | 0x20))
		{
			return true;
		}
	}
	else
	{
		if(c1 == c2)
		{
			return true;
		}
	}
	return false;
}

//*******************************************************************
// Select SD_MMC
static void
select(void)
{
	spi_selectChip(SD_MMC_SPI,1);
}

//*******************************************************************
// Unselect SD_MMC
static void
unselect(void)
{
	spi_unselectChip(SD_MMC_SPI,1);
}

static uint8_t	card_type = 0;		// stores SD_CARD or MMC_CARD type card
static uint8_t	csd[16] = { 0 };	// stores the Card Specific Data

static char		static_xbuf[200] = { 0 };

//*******************************************************************
static unsigned char
send_and_read(
	const unsigned char data_to_send
	)
{
	unsigned short data_read;
	spi_write(SD_MMC_SPI,data_to_send);
	spi_read(SD_MMC_SPI,&data_read);
	return (data_read);
}

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

	spi_write(SD_MMC_SPI,0xFF);            // write dummy byte
	spi_write(SD_MMC_SPI,command | 0x40);  // send command
	spi_write(SD_MMC_SPI,arg>>24);         // send parameter
	spi_write(SD_MMC_SPI,arg>>16);
	spi_write(SD_MMC_SPI,arg>>8 );
	spi_write(SD_MMC_SPI,arg    );
	spi_write(SD_MMC_SPI,0x95);            // correct CRC for first command in SPI (CMD0)
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
//! @brief This function sends a command WITH NO DATA STATE to the SD/MMC and waits for R1 response
//!        This function also selects and unselects the memory => should be used only for single command transmission
//!
//! @warning Code:?? bytes (function code length)
//!
//! @param  command   command to send (see sd_mmc.h for command list)
//!         arg       argument of the command
//!
//! @return U8
//!         R1 response (R1 == 0xFF if time out error)
//!/
static uint8_t
sd_mmc_send_command(uint8_t command, uint32_t arg)
{
	uint8_t	r1;
	select();
	r1 = sd_mmc_command(command, arg);
	unselect();
	return r1;
}

//*******************************************************************
//!
//! @brief This function waits until the SD/MMC is not busy.
//!
//! @warning Code:?? bytes (function code length)
//!
//! @return bit
//!          OK when card is not busy
//!/
static bool
wait_not_busy(void)
{
	uint16_t	retry;
	uint8_t		r1;

	// Select the SD_MMC memory gl_ptr_mem points to
	select();
	retry = 0;
	while ((r1 = send_and_read(0xFF)) != 0xFF) {
		retry++;
		if (retry == 50000) {
			print_dbg("sd_mmc: wait_not_busy timeout.\n");
			goto wait_failure;
		}
	}
	unselect();
	return true;
wait_failure:
	unselect();
	return false;
}

//*******************************************************************
//!
//! @brief This function reads the CSD (Card Specific Data) of the memory card
//!
//! @warning Code:?? bytes (function code length)
//!
//! @param  buffer to fill
//!
//! @return bit
//!         OK / KO
//!/
static bool
read_csd(U8 *buffer)
{
	uint8_t			retry;
	unsigned short	data_read;
	uint8_t			r1;

	// wait for MMC not busy
	if (!wait_not_busy()) {
		goto csd_failure;
	}

	select();
	// issue command
	r1 = sd_mmc_command(MMC_SEND_CSD, 0);
	// check for valid response
	if (r1 != 0x00) {
		print_dbg("sd_mmc: csd command failed.\n");
		goto csd_failure;
	}

	// wait for block start
	retry = 0;
	while ((r1 = send_and_read(0xFF)) != MMC_STARTBLOCK_READ) {
		if (retry > 8) {
			spi_unselectChip(SD_MMC_SPI,1);     // unselect SD_MMC
			return KO;
		}
		retry++;
	}
	for (retry = 0; retry <16; retry++) {
		spi_write(SD_MMC_SPI,0xFF);
		spi_read(SD_MMC_SPI,&data_read);
		buffer[retry] = data_read;
	}
	spi_write(SD_MMC_SPI,0xFF);   // load CRC (not used)
	spi_write(SD_MMC_SPI,0xFF);
	spi_write(SD_MMC_SPI,0xFF);   // give clock again to end transaction
	unselect();
	return true;
csd_failure:
	unselect();
	return false;
}

//*******************************************************************
void
HALayerInit(void)
{
	spi_options_t spiOptions = {
		.reg          = 1,
		.baudrate     = 12*1000000, // Defined in conf_at45dbx.h
		.bits         = 8,         // Defined in conf_at45dbx.h
		.spck_delay   = 0,
		.trans_delay  = 0,
		.stay_act     = 1,
		.spi_mode     = 0,
		.fdiv         = 0,
		.modfdis      = 1
	};

#if (0)
	static const gpio_map_t SD_MMC_SPI_GPIO_MAP = {
		{SD_MMC_SPI_SCK_PIN,  SD_MMC_SPI_SCK_FUNCTION },  // SPI Clock.
		{SD_MMC_SPI_MISO_PIN, SD_MMC_SPI_MISO_FUNCTION},  // MISO.
		{SD_MMC_SPI_MOSI_PIN, SD_MMC_SPI_MOSI_FUNCTION},  // MOSI.
		{SD_MMC_SPI_NPCS_PIN, SD_MMC_SPI_NPCS_FUNCTION}   // Chip Select NPCS.
	};

	// Assign IO to SPI.
	gpio_enable_module(SD_MMC_SPI_GPIO_MAP,
			 sizeof(SD_MMC_SPI_GPIO_MAP) / sizeof(SD_MMC_SPI_GPIO_MAP[0]));

	// Initialize as master.
	spi_initMaster(SD_MMC_SPI, &spiOptions);

	// Set master mode; vxariable_ps, pcs_decode, delay.
	spi_selectionMode(SD_MMC_SPI, 0, 0, 0);

	// Enable SPI.
	spi_enable(SD_MMC_SPI);
#else
	// Rest of the SPI is already initialized by DIP204 driver.
	static const gpio_map_t SD_MMC_SPI_GPIO_MAP = {
		{SD_MMC_SPI_NPCS_PIN, SD_MMC_SPI_NPCS_FUNCTION}   // Chip Select NPCS.
	};

	// Assign IO to SPI.
	gpio_enable_module(SD_MMC_SPI_GPIO_MAP,
			 sizeof(SD_MMC_SPI_GPIO_MAP) / sizeof(SD_MMC_SPI_GPIO_MAP[0]));
#endif

	// Setup register according to spiOptions.
	spi_setupChipReg(SD_MMC_SPI, &spiOptions, F_PBA);
}

/**
 *	checks if there is a memory card in the slot
 *	pulls 
 *
 *	@return	false		card not detected
 *	@return true		car detected
 **/
bool detectCard(void)
{
	return true;
}

/**
 *	sends the command to the the memory card with the 4 arguments and the CRC
 *
 *	@param command		the command
 *	@param argX			the arguments
 *	@param CRC			cyclic redundancy check
 **/
void sendCommand(unsigned char command, unsigned char argA,
                 unsigned char argB, unsigned char argC,
                 unsigned char argD, unsigned char CRC)
{	
	send_and_read(command);
	send_and_read(argA);
	send_and_read(argB);
	send_and_read(argC);
	send_and_read(argD);
	send_and_read(CRC);
}

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
	unsigned char actRes;
	unsigned int count = 256;
	actRes = 0xFF;
	while ((actRes != expRes) && (count > 0)) {
		actRes = send_and_read(0xff);		
		count--;
	}
	if (count == 0)
		return false; 
	return true;
}

/**
 *	initializes the memory card by bring it out of idle state and 
 *	sets it up to use SPI mode
 *
 *	@return true		card successfully initialized
 *	@return false		card not initialized
 **/
bool
memCardInit(void)
{
	uint16_t	retry = 0;
	uint8_t		r1 = 0;

	select();

	// RESET THE MEMORY CARD
	card_type = MMC_CARD;
	retry = 0;
	do {
		// reset card and go to SPI mode
		r1 = sd_mmc_send_command(MMC_GO_IDLE_STATE, 0);
		spi_write(SD_MMC_SPI,0xFF);            // write dummy byte

		// do retry counter
		retry++;
		if(retry > 100) {
			print_dbg("sd_mmc: initial SPI reset timeout.\n");
			goto init_failure;
		}
	} while(r1 != 0x01);   // check memory enters idle_state

	// IDENTIFICATION OF THE CARD TYPE (SD or MMC)
	// Both cards will accept CMD55 command but only the SD card will respond to ACMD41
	r1 = sd_mmc_send_command(SD_APP_CMD55,0);
	spi_write(SD_MMC_SPI,0xFF);  // write dummy byte

	r1 = sd_mmc_send_command(SD_SEND_OP_COND_ACMD, 0);
	spi_write(SD_MMC_SPI,0xFF);  // write dummy byte

	if ((r1&0xFE) == 0) {   // ignore "in_idle_state" flag bit
		card_type = SD_CARD;    // card has accepted the command, this is a SD card
		print_dbg("sd_mmc: detected SD card.\n");
	} else {
		card_type = MMC_CARD;   // card has not responded, this is a MMC card
		print_dbg("sd_mmc: querying MMC card.\n");
		// reset card again
		retry = 0;
		do {
			// reset card again
			r1 = sd_mmc_send_command(MMC_GO_IDLE_STATE, 0);
			spi_write(SD_MMC_SPI,0xFF);            // write dummy byte
			// do retry counter
			retry++;
			if (retry > 100) {
				print_dbg("sd_mmc: reset timeout.\n");
				goto init_failure;
			}
		} while(r1 != 0x01);   // check memory enters idle_state
	}

	// CONTINUE INTERNAL INITIALIZATION OF THE CARD
	// Continue sending CMD1 while memory card is in idle state
	retry = 0;
	do {
		// initializing card for operation
		r1 = sd_mmc_send_command(MMC_SEND_OP_COND, 0);
		spi_write(SD_MMC_SPI,0xFF);            // write dummy byte
		// do retry counter
		retry++;
		if (retry == 50000) {    // measured approx. 500 on several cards
			print_dbg("sd_mmc: init timeout.\n");
			goto init_failure;
		}
	} while (r1);

	// DISABLE CRC TO SIMPLIFY AND SPEED UP COMMUNICATIONS
	r1 = sd_mmc_send_command(MMC_CRC_ON_OFF, 0);  // disable CRC (should be already initialized on SPI init)
	spi_write(SD_MMC_SPI,0xFF);            // write dummy byte

	// SET BLOCK LENGTH TO 512 BYTES
	r1 = sd_mmc_send_command(MMC_SET_BLOCKLEN, 512);
	spi_write(SD_MMC_SPI,0xFF);            // write dummy byte
	if (r1 != 0x00) {
		print_dbg("sd_mmc: card unsupported because it doesn't support block size 512.\n");
		goto init_failure; // card unsupported if block length of 512b is not accepted
	}

	// GET CARD SPECIFIC DATA
	if (!read_csd(csd)) {
		goto init_failure;
	}

#if (FATLOG)
	{
		unsigned int i;
		print_dbg("sd_mmc: csd=");
		for (i=0; i<16; ++i) {
			sprintf(static_xbuf, " %02X", csd[i]);
			print_dbg(static_xbuf);
		}
		print_dbg("\n");
	}
#endif

	unselect();
	print_dbg("sd_mmc: memCardInit INIT OK.\n");
	return true;
init_failure:
	unselect();
	print_dbg("sd_mmc: memCardInit FAILED.\n");
	return false;
}

/**
 *	Initialize the block length in the memory card to be 512 bytes
 *	
 *	return true			block length sucessfully set
 *	return false		block length not sucessfully set
 **/
bool
setBLockLength(void)
{
	bool r = false;

	select();

	sendCommand(CMD16,0,0,2,0,0xFF);
	r = cardResponse(0x00);
	send_and_read(0xFF);

	unselect();

	if (!r) {
		print_dbg("sd_mmc: setBlockLength failed.\n");
	}
	return r;
}

/**
 *	locates the offset of the partition table from the absolute zero sector
 *
 *	return ...			the offset of the partition table
 **/
unsigned long
getPartitionOffset(void)
{
	unsigned int count = 0;
	unsigned long offset = 0;
	unsigned char charBuf;
	
	select();

	sendCommand(CMD17,0,0,0,0,0xFF);
	if(cardResponse(0x00))
	{
		if(cardResponse(0xFE))
		{
			for(count = 0; count < 454; count++)
				send_and_read(0xff);
			charBuf = send_and_read(0xff);
			offset = charBuf;
			charBuf = send_and_read(0xff);
			offset |= (unsigned long int)charBuf << 8;
			charBuf = send_and_read(0xff);
			offset |= (unsigned long int)charBuf << 16;
			charBuf = send_and_read(0xff);
			offset += (unsigned long int)charBuf << 24;
			for(count = 458; count < SECTOR_SIZE; count++)
				send_and_read(0xff);
			send_and_read(0xff);
			send_and_read(0xff);
		}
	}
	
	send_and_read(0xff);

	unselect();
	
#if (FATLOG)
	{
		sprintf(static_xbuf, "sd_mmc: getPartitionOffset=%ld\n", offset);
		print_dbg(static_xbuf);
	}
#endif
	return offset;
}

/**
 *	checks to see if previous write operation was successful
 *	
 *	@return true		if data is successfully written to disk
 *	@return false		if data is not successfully written to disk
 **/
static bool checkWriteState(void)
{
	unsigned int count = 0;
	char inRes;

	while (count <= 64) {
		inRes = send_and_read(0xff) & 0x1F;
		if (inRes==0x05) {
			break;
		}
		if (inRes == 0x0B || inRes == 0x0D) {
			return false;
		}
		count++;
	}

	count = 0;
	while ((count<64) && (send_and_read(0xff)==0x00)) {
		++count;
	}

	return count<64;
}

/**
 *	read content of the sector indicated by the input and place it in the buffer
 *
 *	@param	sector		sector to be read
 *	@param	*buf		pointer to the buffer
 *
 *	@return	...			number of bytes read
 **/
unsigned int readSector(unsigned long sector, unsigned char *buf)
{
    unsigned int count = 0;
	bool		ok = false;
	sector += sectorZero;
	sector *= 2;	
	
#if (FATLOG)
	{
		sprintf(static_xbuf, "readSector 0x%04lX\n", sector);
		print_dbg(static_xbuf);
	}
#endif
	select();

	sendCommand(CMD17,(sector>>16)&0xFF,(sector>>8)&0xFF,sector&0xFF,0,0xFF);
	if (cardResponse(0x00))
	{
		if (cardResponse(0xFE))
		{
			for(count = 0; count < SECTOR_SIZE; count++)
			{
				buf[count] = send_and_read(0xff);
			}
			send_and_read(0xff);
			send_and_read(0xff);
			ok = true;
		} else {
			sprintf(static_xbuf, "readSector: failed 1 to read sector %04lX.\n", sector);
			print_dbg(static_xbuf);
		}
	} else {
		sprintf(static_xbuf, "readSector: failed 2 to read sector %04lX.\n", sector);
		print_dbg(static_xbuf);
	}
	
	send_and_read(0xff);

	unselect();

	if (false) {
		sprintf(static_xbuf, "%04lX: %02X %02X %02X %02X %02X %02X %02X %02X\n",
				sector, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
		print_dbg(static_xbuf);
	}
	return count;
}

/**
 *	read partial content of the sector indicated by the input and place it in the buffer
 *
 *	@param	sector		sector to be read
 *	@paran	*buf		pointer to the buffer
 *	@param	bytesOffset	starting point in the sector for reading
 *	@param	bytesToRead	number of bytes to read
 *
 *	@return	...			number of bytes read
 **/
unsigned int readPartialSector(unsigned long sector,
                               unsigned int byteOffset,
                               unsigned int bytesToRead,
                               unsigned char *buf)
{
    int count = 0;
	int countRead = 0;
	int leftover = SECTOR_SIZE - byteOffset - bytesToRead;
	sector += sectorZero;
	sector *= 2;	
	
#if (FATLOG)
	{
		sprintf(static_xbuf, "readPartialSector 0x%04lX 0x%04X 0x%04X\n", sector, byteOffset, bytesToRead);
		print_dbg(static_xbuf);
	}
#endif

	select();
	sendCommand(CMD17,(sector>>16)&0xFF,(sector>>8)&0xFF,sector&0xFF,0,0xFF);
	if(cardResponse(0x00))
	{
		if(cardResponse(0xFE))
		{
			for(count = 0; count < byteOffset; count++)
				send_and_read(0xff);
			for(countRead = 0; countRead < bytesToRead; countRead++)
			{
				buf[countRead] = send_and_read(0xff);
			}
			for(count = 0; count < leftover; count++)
				send_and_read(0xff);
			send_and_read(0xff);
			send_and_read(0xff);
		}
	}
	
	send_and_read(0xff);
	return countRead;
}

/**
 *	read partial content at the end of the sector1 indicated by the input and
 *	the begging of sector 2 and place it in the buffer
 *
 *	@param	sector1		first sector to be read
 *	@param	sector2		second sector to be read
 *	@paran	*buf		pointer to the buffer
 *	@param	bytesOffset	starting point in the sector for reading
 *	@param	bytesToRead	number of bytes to read
 *
 *	@return	...			number of bytes read
 **/
unsigned int readPartialMultiSector(unsigned long sector1,
                                    unsigned long sector2,
                                    unsigned int byteOffset,
                                    unsigned int bytesToRead,
                                    unsigned char *buf)
{
	int count = 0;
	int countRead = 0;
	sector1 += sectorZero;
	sector1 *= 2;	
	sector2 += sectorZero;
	sector2 *= 2;	
	
#if (FATLOG)
	{
		sprintf(static_xbuf, "readPartialMultiSector 0x%04lX,0x%04lX 0x%04X 0x%04X\n", sector1, sector2, byteOffset, bytesToRead);
		print_dbg(static_xbuf);
	}
#endif

	sendCommand(CMD17,(sector1>>16)&0xFF,(sector1>>8)&0xFF,sector1&0xFF,0,0xFF);
	if(cardResponse(0x00))
	{
		if(cardResponse(0xFE))
		{
			for(count = 0; count < byteOffset; count++)
				send_and_read(0xff);
			for(count = byteOffset; count < SECTOR_SIZE; count++, countRead++)
			{
				buf[countRead] = send_and_read(0xff);
			}
			send_and_read(0xff);
			send_and_read(0xff);
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
	
	send_and_read(0xff);
	send_and_read(0xff);
	
	sendCommand(CMD17,(sector2>>16)&0xFF,(sector2>>8)&0xFF,sector2&0xFF,0,0xFF);
	if(cardResponse(0x00))
	{
		if(cardResponse(0xFE))
		{			
			for(count = 0; countRead < bytesToRead; count++, countRead++)
			{
				buf[countRead] = send_and_read(0xff);
			}
			for(count = count; count < SECTOR_SIZE; count++)
				send_and_read(0xff);
			send_and_read(0xff);
			send_and_read(0xff);
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
	
	send_and_read(0xff);
    return countRead;
}

/**
 *	writes content of the buffer to the sector indicated by the input
 *
 *	@param	sector		sector to be written
 *	@param	*buf		pointer to the buffer
 *
 *	@return	...			number of bytes written
 **/
unsigned int writeSector(unsigned long sector, unsigned char *buf)
{
    unsigned int count = 0;
	sector += sectorZero;
	sector *= 2;	
	
#if (FATLOG)
	{
		sprintf(static_xbuf, "writeSector 0x%04lX\n", sector);
		print_dbg(static_xbuf);
	}
#endif

	select();

	sendCommand(CMD24,(sector>>16)&0xFF,(sector>>8)&0xFF,sector&0xFF,0,0xFF);
	if(cardResponse(0x00))
	{
		send_and_read(0xff);
		send_and_read(0xfe);
		for(count = 0; count < SECTOR_SIZE; count++)
		{
			send_and_read(buf[count]);
		}
		send_and_read(0xff);
		send_and_read(0xff);
		checkWriteState();
	}

	send_and_read(0xff);

	unselect();

	wait_not_busy();

	return count;
}

/**
 *	write the content of the buffer indicated by the input to the MMC/SD card location
 *	indicated by the input sector.
 *
 *	@param	sector		sector to be written to
 *	@paran	*buf		pointer to the buffer
 *	@param	bytesOffset	starting point in the sector for writing
 *	@param	bytesToRead	number of bytes to write
 *
 *	@return	...			number of bytes written
 **/
unsigned int writePartialSector(unsigned long sector,
                                unsigned int byteOffset,
                                unsigned int bytesToWrite,
                                unsigned char *buf)
{
    int count = 0;
	int countWrote = 0;	
	unsigned char readBuf[SECTOR_SIZE];

#if (FATLOG)
	{
		sprintf(static_xbuf, "writePartialSector 0x%04lX 0x%04x 0x%04X\n", sector, byteOffset, bytesToWrite);
		print_dbg(static_xbuf);
	}
#endif

	readSector(sector,readBuf);
	sector += sectorZero;
	sector *= 2;	
	
	select();

	sendCommand(CMD24,(sector>>16)&0xFF,(sector>>8)&0xFF,sector&0xFF,0,0xFF);
	if(cardResponse(0x00))
	{
		send_and_read(0xff);
		send_and_read(0xfe);
		for(count = 0; count < byteOffset; count++)		
			send_and_read(readBuf[count]);
		for(countWrote = 0; countWrote < bytesToWrite; countWrote++, count++)
		{
			send_and_read(buf[countWrote]);
		}
		for(count = count; count < SECTOR_SIZE; count++)
			send_and_read(readBuf[count]);
		send_and_read(0xff);
		send_and_read(0xff);
		checkWriteState();
	}
	
	send_and_read(0xff);

	unselect();

	wait_not_busy();

	return countWrote;
}

/**
 *	write the content of the buffer to the end of the sector1
 *	and the begging of sector 2
 *
 *	@param	sector1			first sector to be written to
 *	@param	sector2			second sector to be written to
 *	@paran	*buf			pointer to the buffer
 *	@param	bytesOffset		starting point in the sector for reading
 *	@param	bytesToWrite	number of bytes to write
 *
 *	@return	...				number of bytes written
 **/
unsigned int writePartialMultiSector(unsigned long sector1,
                                     unsigned long sector2,
                                     unsigned int byteOffset,
                                     unsigned int bytesToWrite,
                                     unsigned char *buf)
{
    unsigned int count = 0;
	unsigned int countWrote = 0;	
	unsigned int writeSector2 = bytesToWrite - SECTOR_SIZE + byteOffset;
	unsigned char readBuf[SECTOR_SIZE];

#if (FATLOG)
	{
		sprintf(static_xbuf, "writePartialMultiSector 0x%04lX,0x%04lX 0x%04x 0x%04X\n", sector1, sector2, byteOffset, bytesToWrite);
		print_dbg(static_xbuf);
	}
#endif
	readSector(sector1,readBuf);

	sector1 += sectorZero;
	sector1 *= 2;	
	
	select();
	sendCommand(CMD24,(sector1>>16)&0xFF,(sector1>>8)&0xFF,sector1&0xFF,0,0xFF);
	if(cardResponse(0x00))
	{
		send_and_read(0xff);
		send_and_read(0xfe);
		for(count = 0; count < byteOffset; count++)		
			send_and_read(readBuf[count]);
		for(count = byteOffset; count < SECTOR_SIZE; countWrote++, count++)
		{
			send_and_read(buf[countWrote]);
		}
		send_and_read(0xff);
		send_and_read(0xff);
		checkWriteState();
	}
	
	send_and_read(0xff);
	unselect();

	wait_not_busy();

	readSector(sector2,readBuf);

	sector2 += sectorZero;
	sector2 *= 2;
	
	select();

	sendCommand(CMD24,(sector2>>16)&0xFF,(sector2>>8)&0xFF,sector2&0xFF,0,0xFF);
	if(cardResponse(0x00))
	{
		send_and_read(0xff);
		send_and_read(0xfe);
		for(count = 0; count < writeSector2; countWrote++, count++)
		{
			send_and_read(buf[countWrote]);
		}
		for(count = writeSector2; count < SECTOR_SIZE; count++)
			send_and_read(readBuf[count]);		
		send_and_read(0xff);
		send_and_read(0xff);
		checkWriteState();
	}
	
	send_and_read(0xff);
	send_and_read(0xff);

	unselect();

	wait_not_busy();

	return countWrote;
}

