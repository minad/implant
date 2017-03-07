#ifndef _util_h
#define _util_h

#include <vector>
#include <string>
#include <exception>

namespace util {

std::string w_readlink(const std::string&);
void w_chown(const std::string&, uid_t, gid_t);
void w_unlink(const std::string&);
void w_remove(const std::string&);
void w_rmdir(const std::string&);
void w_symlink(const std::string&, const std::string&);
void w_mkdir(const std::string&, mode_t);
void w_readdir(const std::string&, std::vector<std::string>&);

std::string operator/(const std::string&, const std::string&);
std::string dirname(const std::string&);
std::string collapse(const std::string&); 

class SystemError : public std::exception {
public:
	explicit SystemError(const std::string&);
	explicit SystemError(const std::string&, int);
	virtual  ~SystemError() throw();
	virtual const char* what() const throw();
private:
	std::string msg;
};

}

#endif
