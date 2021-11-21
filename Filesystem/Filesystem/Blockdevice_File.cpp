/**
vim: ts=4
vim: shiftwidth=4
*/
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <Filesystem/Blockdevice_File.h>
#include <Filesystem/Error.h>

#include <Filesystem_Config.h>


namespace Filesystem {

//*******************************************************************
Blockdevice_File::Blockdevice_File(
	const std::string&	filename
)
:	read_count_(0)
	,write_count_(0)
{
	f_ = fopen(filename.c_str(), "rb+");
	if (f_ == 0) {
		throw Error("Failed to open file '%s' for read-write", filename.c_str());
	}
	fseek(f_, 0, SEEK_END);
	max_block_nr = ftell(f_) / BLOCK_SIZE;
}

//*******************************************************************
Blockdevice_File::~Blockdevice_File()
{
	if (f_ != 0) {
		fclose(f_);
		filesystem_dprintf(("Blockdevice_File: statistics: %d reads, %d writes\n", read_count_, write_count_));
	}
}

//*******************************************************************
bool
Blockdevice_File::Read(
	const unsigned int	nr,
	void*				block
)
{
	filesystem_dprintf(("Blockdevice_File::Read: 0x%04X\n", nr));

	if (nr > max_block_nr) {
		throw Error("Blockdevice_File::Read: block %d is out of range [0 ... %d).", nr, max_block_nr);
	}
	const int	r1 = fseek(f_, nr*BLOCK_SIZE, SEEK_SET);
	if (r1 == 0) {
		const size_t	r2 = fread(block, BLOCK_SIZE, 1, f_);
		if (r2 == 1) {
			++read_count_;
			return true;
		} else {
			filesystem_dprintf(("Blockdevice_File: fread failed, return value %d", r2)); 
		}
	} else {
		filesystem_dprintf(("Blockdevice_File: fseek failed, return value %d", r1)); 
	}
	return false;
}

//*******************************************************************
bool
Blockdevice_File::Write(
	const unsigned int	nr,
	const void*			block
)
{
	filesystem_dprintf(("Blockdevice_File::Write: 0x%04X\n", nr));

	if (nr > max_block_nr) {
		throw Error("Blockdevice_File::Write: block %d is out of range [0 ... %d).", nr, max_block_nr);
	}
	const int	r1 = fseek(f_, nr*BLOCK_SIZE, SEEK_SET);
	if (r1 == 0) {
		const size_t	r2 = fwrite(block, BLOCK_SIZE, 1, f_);
		if (r2 == 1) {
			++write_count_;
			return true;
		} else {
			filesystem_dprintf(("Blockdevice_File: fwrite failed, return value %d", r2)); 
		}
	} else {
		filesystem_dprintf(("Blockdevice_File: fseek failed, return value %d", r1)); 
	}

	return false;
}

} // namespace Filesystem
