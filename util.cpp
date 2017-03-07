#include "util.h"
#include <sys/param.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdexcept>

namespace util {

using std::string;
using std::vector;
using std::runtime_error;

string w_readlink(const string& path) {
	char buffer[MAXPATHLEN];
	ssize_t count = readlink(path.c_str(), buffer, sizeof (buffer));
	if (count < 0)
		throw SystemError("Cannot read link " + path);
	buffer[count] = 0;
	if (buffer[0] =='/')
		return collapse(buffer); // remove trailing slashes	
	return dirname(path) / buffer;
}

void w_chown(const string& path, uid_t uid, gid_t gid) {
	if (chown(path.c_str(), uid, gid) < 0)
		throw SystemError("Cannot change owner of " + path);
}

void w_unlink(const string& path) {
	if (unlink(path.c_str()) < 0)
		throw SystemError("Cannot unlink " + path);
}

void w_remove(const string& path) {
	if (remove(path.c_str()) < 0)
		throw SystemError("Cannot remove " + path);
}

void w_rmdir(const string& path) {
	if (rmdir(path.c_str()) < 0)
		throw SystemError("Cannot remove directory " + path);
}

void w_symlink(const string& src, const string& dst) {
	if (symlink(src.c_str(), dst.c_str()) < 0)
		throw SystemError("Cannot create link " + src + " -> " + dst); 
}

void w_mkdir(const string& path, mode_t mode) {
	if (mkdir(path.c_str(), mode) < 0)
		throw SystemError("Cannot create directory " + path);
}

void w_readdir(const string& path, vector<string>& list) {
	DIR* dir = opendir(path.c_str());
	if (!dir)
		throw SystemError("Cannot open directory " + path);
	dirent* entry;
	while (entry = readdir(dir)) {
		// ".", ".." are ignored
		if (entry->d_name[0] == '.') {
			if (!entry->d_name[1])
				continue;
			if (entry->d_name[1] == '.' && !entry->d_name[2])
				continue;
		}
		list.push_back(entry->d_name);
	}
	closedir(dir);
}

string operator/(const string& a, const string& b) {
	return collapse(a + "/" + b);
}

string dirname(const string& path) {
	string p = collapse(path);
	size_t i;
	for (i = p.size(); i > 1; --i) {
		if (p[i - 1] != '/')
			break;
	} 
	p = p.substr(0, i);
	i = p.rfind('/');
	if (i == 0)
		return "/";
	if (i == string::npos)
		return "";
	return p.substr(0, i);
}

string collapse(const string& p) {
	string path;
	for (size_t i = 0; i < p.size(); ++i) {
		if (p[i] == '/') {
			if (i+2 < p.size() && p[i+1] == '.') {
				if (p[i+2] == '/') {
					i += 3;
				} else if (i+3 < p.size() &&
                                           p[i+2] == '.' &&
				           p[i+3] == '/') {
					if (path != "/") {
						size_t n = path.rfind('/', path.size() - 2);
						if (n == string::npos)
							n = 0;
						path.erase(n+1);
					}
					i += 3;
					continue;
				}
			}
			if (path.size() == 0 || path[path.size()-1] != '/')
				path += '/';
		} else
			path += p[i];
	}
	if (path.size() > 1 && path[path.size() - 1] == '/')
		path.erase(path.size() - 1, 1);
	
	return path;
}

SystemError::SystemError(const string& m)
	: msg(m + ": " + strerror(errno)) {
}

SystemError::~SystemError() throw() {
}

const char* SystemError::what() const throw() {
	return msg.c_str();
}

}

