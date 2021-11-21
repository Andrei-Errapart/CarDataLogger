/**
vim: ts=4
vim: shiftwidth=4
*/
#ifndef Config_h_
#define	Config_h_

#include <Filesystem/FAT16.h>
#include <Filesystem/File.h>

namespace Filesystem {

/** Simple-minded configuration file interface. */
class Config {
public:
	/** Load the configuration file. */
	Config(
		Filesystem::FAT16&	filesys,
		const char*			filename,
		char*				buffer,
		const unsigned int	buffer_size
	);

	/** Get the value stored in the configuration file. The contents may be modified. */
	char*
	Value(
		const char*	section,
		const char*	key
		);

	/** Get the value stored in the configuration file. The contents may not be modified :) */
	const char*
	Value(
		const char*	section,
		const char*	key,
		const char*	default_value
		);

	/** Get the integer value stored in the configuration file.*/
	int 
	ValueAsInt(
		const char*	section,
		const char*	key,
		const int	default_value
		);
private:
	typedef enum {
		TYPE_SECTION,
		TYPE_KEY,
		TYPE_VALUE
	} TYPE;

	typedef struct {
		TYPE	type;
		char*	text;
	} TOKEN;

	char*				buffer_;
	const unsigned int	buffer_size_;
	TOKEN*				tokens_;
	unsigned int		tokens_size_;
}; // class Config

} // namespace Filesystem

#endif /* Config_h_ */

