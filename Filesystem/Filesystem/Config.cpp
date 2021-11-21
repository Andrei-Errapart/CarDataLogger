/**
vim: ts=4
vim: shiftwidth=4
*/
#include <Filesystem/FAT16.h>	// Filesystem.
#include <Filesystem/Config.h>	// Configuration file.

#include <string.h>				// memset, strcasecmp.
#include <ctype.h>				// isalpha, etc.
#include <stdlib.h>				// atoi.

#if defined(_MSC_VER)
#define strcasecmp	stricmp
#endif

namespace Filesystem {

//*******************************************************************
/** file parsing state. */
typedef enum {
	MODE_DEFAULT,
	MODE_SECTION,
	MODE_KEY,
	MODE_VALUE
} MODE;

//*******************************************************************
Config::Config(
	Filesystem::FAT16&	filesys,
	const char*			filename,
	char*				buffer,
	const unsigned int	buffer_size
) :
	buffer_(buffer)
	,buffer_size_(buffer_size)
	,tokens_(0)
	,tokens_size_(0)
{
	// 1. Read the file into buffer_.
	Filesystem::File	f(filesys, filename, Filesystem::OPEN_READONLY);
	const unsigned int	filesize = f.Size();
	const unsigned int	nread = filesize < buffer_size_ ? filesize : buffer_size_;

	memset(buffer_, 0, buffer_size);
	f.Read(buffer_, nread);

	// 2. Set up token buffer. */
	tokens_size_ = (buffer_size - 1 - nread) / sizeof(TOKEN);
	tokens_ = reinterpret_cast<TOKEN*>(buffer_ + buffer_size_ - tokens_size_*sizeof(TOKEN));

	// Tokenize the buffer :)
	unsigned int	token_index = 0;
	char*			ptr = buffer;
	char*			text = buffer;
	MODE			mode = MODE_DEFAULT;

	for (; *ptr!=0; ++ptr) {
		const char	c = *ptr;
		switch (mode) {
		case MODE_DEFAULT:
			if (c=='[') {
				mode = MODE_SECTION;
				text = ptr + 1;
			} else if (isalpha(*ptr)) {
				mode = MODE_KEY;
				text = ptr;
			} else {
				// do what?
			}
			break;
		case MODE_SECTION:
			if (c==']') {
				// end of section.
				*ptr = 0;
				mode = MODE_DEFAULT;
				if (token_index < tokens_size_) {
					tokens_[token_index].type = TYPE_SECTION;
					tokens_[token_index].text = text;
					++token_index;
				}
			} else if (c==0x0D || c==0x0A) {
				mode = MODE_DEFAULT;
			}
			break;
		case MODE_KEY:
			if (c=='=') {
				*ptr = 0;
				if (token_index < tokens_size_) {
					tokens_[token_index].type = TYPE_KEY;
					tokens_[token_index].text = text;
					++token_index;
				}
				mode = MODE_VALUE;
				text = ptr + 1;
			} else if (c==0x0D || c==0x0A) {
				mode = MODE_DEFAULT;
			}

			break;
		case MODE_VALUE:
			if (c==0x0D || c==0x0A) {
				*ptr = 0;
				if (token_index < tokens_size_) {
					tokens_[token_index].type = TYPE_VALUE;
					tokens_[token_index].text = text;
					++token_index;
				}
				mode = MODE_DEFAULT;
			}
			break;
		}
	}
}

//*******************************************************************
char*
Config::Value(
	const char*	section,
	const char*	key
)
{
	bool	section_found = false;
	bool	key_found = false;

	for (unsigned int i=0; i<tokens_size_; ++i) {
		switch (tokens_[i].type) {
		case TYPE_SECTION:
			section_found = strcasecmp(section, tokens_[i].text)==0;
			key_found = false;
			break;
		case TYPE_KEY:
			key_found = strcasecmp(key, tokens_[i].text)==0;
			break;
		case TYPE_VALUE:
			if (section_found && key_found) {
				return tokens_[i].text;
			}
			key_found = false;
			break;
		}
	}
	return 0;
}

//*******************************************************************
const char*
Config::Value(
	const char*	section,
	const char*	key,
	const char*	default_value
	)
{
	const char*	r = Value(section, key);
	return r==0 ? default_value : r;
}

//*******************************************************************
int 
Config::ValueAsInt(
	const char*	section,
	const char*	key,
	const int	default_value
)
{
	const char*	r = Value(section, key);
	return r==0 ? default_value : atoi(r);
}

} // namespace Filesystem

