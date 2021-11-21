/**
vim: ts=4
vim: shiftwidth=4
*/
#include <stdarg.h>
#include <stdio.h>
#include <Filesystem_Config.h>
#include <Filesystem/Error.h>

namespace Filesystem {

//*******************************************************************
Error::Error(
		const char* fmt,
		...
)
{
	va_list	ap;
	va_start(ap, fmt);
	const int	n = vsnprintf(what_, sizeof(what_)-1, fmt, ap); 
	va_end(ap);

	if (n>=0 && n<static_cast<int>(sizeof(what_))) {
		what_[n] = 0;
	} else {
		what_[sizeof(what_)-1] = 0;
	}

	filesystem_dprintf(("Exception: %s \n", what_));
}

//*******************************************************************
const char*
Error::what() const throw()
{
	return what_;
}

//*******************************************************************
Error::~Error() throw()
{
}

} // namespace Filesystem
