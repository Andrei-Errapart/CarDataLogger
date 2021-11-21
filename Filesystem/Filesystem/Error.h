/**
vim: ts=4
vim: shiftwidth=4
*/
#ifndef Filesystem_Error_h_
#define Filesystem_Error_h_

#include <stdexcept>
#include <exception>

namespace Filesystem {

class Error : public std::exception {
private:
	char	what_[256];
public:
	Error(
		const char* fmt,
		...
	);
	virtual const char*	what() const throw();
	virtual ~Error() throw();
}; // class Error

} // namespace Filesystem

#endif /* Filesystem_Error_h_ */
