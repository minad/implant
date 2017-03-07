#include "util.h"
#include "color.h"
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <iostream>
#include <vector>
#include <set>

using std::string;
using std::vector;
using std::set;
using std::cout;
using std::cerr;
using std::endl;
using namespace util;

// getopt-Options
const struct option longOptions[] = {
	{ "masked",       1, NULL, 'm' },
	{ "color",        0, NULL, 'c' },
	{ "version",      0, NULL, 'V' },
	{ "help",         0, NULL, 'h' },
	{ NULL,           0, NULL, 0   },
};

// Help string
const char *help =
"Implant Check Version " VERSION "\n"
"Usage: implant-check [Options] PkgDir\n"
"Options:\n"
"  -m DIR, --masked=DIR  Add DIR to masked list\n"
"  -c, --color           Colored output\n"
"  -V, --version         Show Implant version number\n"
"  -h, --help            Show this help\n";

bool _colorEnabled = false;

set<string> masked;
string pkgDir;

void check(string path) {
	if (masked.find(path) != masked.end())
		return;
	struct stat st;
	if (lstat(path.c_str(), &st) < 0)
		throw SystemError("Lstat on " + path + " failed");
	if (S_ISLNK(st.st_mode)) {
		if (stat(path.c_str(), &st) < 0)
			cout << FG_RED << "Dead link:    " << NOCOLOR << path << endl;
		else {
			string link = w_readlink(path);
			if (!S_ISDIR(st.st_mode)) {
				if (link.find(pkgDir) != 0)
					cout << FG_MAGENTA << "Link to file: " << NOCOLOR << path << " -> " << link << endl;
				return;
			}
			if (link.find(pkgDir) == 0)
				return;
			cout << FG_GREEN << "Link to dir:  " << NOCOLOR << path << " -> " << link << endl;			
		}
	} else if (!S_ISDIR(st.st_mode)) { 
		cout << FG_MAGENTA << "File:         " << NOCOLOR << path << endl;
	} else {
		vector<string> child;
		w_readdir(path, child);		
	
		if (child.size() == 0) {
			cout << FG_MAGENTA << "Empty dir:    " << NOCOLOR << path << endl;
			return;
		}

		for (size_t i = 0; i < child.size(); ++i)
			check(path / child[i]);
	}
}

int main(int argc, char* argv[]) {
	try {
		for (;;) {
			int option = getopt_long(argc, argv, "m:cVh?", longOptions, NULL);
			if (option == EOF)
				break;
			switch(option) {
			case 'm':
				masked.insert(optarg);
				break;
			case 'c':
				_colorEnabled = true;
				break;	
			case 'V':
				cout << "Implant Version " VERSION "\n" << endl;
				exit(0);
			case 'h':
				cout << help << endl;
				exit(0);
			case '?':
				cerr << help << endl;
				exit(1);
			}
		}

		if (optind >= argc) {
			cerr << argv[0] << " -- missing package directory\n" << help << endl;
			exit(1);
		}

		pkgDir = collapse(argv[optind]);

		masked.insert(pkgDir);
		masked.insert("/dev");
		masked.insert("/home");
		masked.insert("/proc");
		masked.insert("/sys");
		masked.insert("/tmp");		

		check("/");

		exit(0);
	} catch (std::exception& ex) {
		cerr << FG_RED << "ERROR" << NOCOLOR << ": " << ex.what() << endl;
		exit(1);
	}
}

