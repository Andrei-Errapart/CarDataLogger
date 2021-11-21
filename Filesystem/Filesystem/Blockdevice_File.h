/**
vim: ts=4
vim: shiftwidth=4
*/
#ifndef Filesystem_Blockdevice_File_h_
#define Filesystem_Blockdevice_File_h_


/** \file File-based block device function. */

#include <string>	// std::string
#include <stdio.h>	// FILE*

#include <Filesystem/Blockdevice.h>

namespace Filesystem {

class Blockdevice_File : public Blockdevice {
public:
	Blockdevice_File(
		const std::string&	filename
	);
	virtual ~Blockdevice_File();

	virtual bool Read(
		const unsigned int	nr,
		void*				block
	);

	virtual bool Write(
		const unsigned int	nr,
		const void*			block
	);
private:
	FILE*			f_;
	unsigned int	max_block_nr;
	unsigned int	read_count_;
	unsigned int	write_count_;
}; // class Blockdevice_File

} // namespace Filesystem

#endif /* Filesystem_Blockdevice_File_h_ */
