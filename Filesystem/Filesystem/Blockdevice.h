/**
vim: ts=4
vim: shiftwidth=4
*/
#ifndef Filesystem_Blockdevice_h_
#define Filesystem_Blockdevice_h_


namespace Filesystem {

/** Block device abstraction. */
class Blockdevice {
public:
	enum {
		/** Size of blocks. */
		BLOCK_SIZE	= 512
	};

	/** Virtual destructor :) */
	virtual ~Blockdevice();

	virtual bool
	Read(
		const unsigned int	nr,
		void*				block
	) = 0;

	virtual bool
	Write(
		const unsigned int	nr,
		const void*			block
	) = 0;
}; // class Blockdevice

} // namespace Filesystem


#endif /* Filesystem_Blockdevice_h_ */
