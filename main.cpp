#include "implant.h"
#include "util.h"
#include "color.h"
#include <unistd.h>
#include <getopt.h>
#include <iostream>

// getopt-Options
const struct option longOptions[] = {
	{ "pretend",      0, NULL, 'p' },
	{ "dir",          1, NULL, 'd' },
	{ "target",       1, NULL, 't' },
	{ "delete",       0, NULL, 'D' },
	{ "verbose",      0, NULL, 'v' },
	{ "color",        0, NULL, 'c' },
	{ "version",      0, NULL, 'V' },
	{ "help",         0, NULL, 'h' },
	{ NULL,           0, NULL, 0   },
};

// Help string
const char *help =
"Implant Version " VERSION "\n"
"Usage: implant [Options] Packages...\n"
"Options:\n"
"  -p, --pretend         Do not actually make changes\n"
"  -d DIR, --dir=DIR     Set package dir to DIR (default is current dir)\n"
"  -t DIR, --target=DIR  Set target to DIR (default is parent of package dir)\n"
"  -D, --delete          Delete package links from target\n"
"  -v, --verbose         Increase verbosity\n"
"  -c, --color           Colored output\n"
"  -V, --version         Show Implant version number\n"
"  -h, --help            Show this help\n";

bool _colorEnabled = false;

inline const char* prefix(bool pretend) {
	return (pretend ? "===== PRETEND: " : "===== ");
}

int main(int argc, char* argv[]) {
	try {
		bool pretend = false, remove = false;
		std::string dir, target;
		int verbose = 0;
		for (;;) {
			int option = getopt_long(argc, argv, "pd:t:DvcVh?", longOptions, NULL);
			if (option == EOF)
				break;
			switch(option) {
			case 'p':
				pretend = true;
				break;
			case 'd':
				dir = optarg;
				break;
			case 't':
				target = optarg;
				break;
			case 'D':
				remove = true;
				break;
			case 'v':
				++verbose;
				break;
			case 'c':
				_colorEnabled = true;
				break;	
			case 'V':
				std::cout << "Implant Version " VERSION "\n" << std::endl;
				exit(0);
			case 'h':
				std::cout << help << std::endl;
				exit(0);
			case '?':
				std::cerr << help << std::endl;
				exit(1);
			}
		}

		if (optind >= argc) {
			std::cerr << argv[0] << " -- missing package names\n" << help << std::endl;
			exit(1);
		}
	
		if (!dir.size()) {
			char buffer[255];
			dir = getcwd(buffer, sizeof (buffer));
		}

		if (!target.size())
			target = util::dirname(dir);

		implant::Implant implant(dir, target, verbose);

		if (!pretend) {
			std::cout << FG_RED << "\aWARNING: Did you check any changes with --pretend?\n"
					       "         Sleeping for 2 seconds..." << NOCOLOR << std::endl; 
			usleep(2000000);
		}

		for (int i = optind; i < argc; ++i) {
			int conflicts;
			bool alreadyDone, blocked;
			if (remove) {
				std::cout << prefix(pretend) << "Removing " << argv[i] << " =====" << std::endl;
				implant.remove(argv[i], conflicts, alreadyDone); 
				if (conflicts) {
					std::cout << prefix(pretend) << FG_RED << conflicts << " Conflicts" << NOCOLOR << " =====" << std::endl;
				} else if (alreadyDone) {
					std::cout << prefix(pretend) << FG_LTGREEN << "Already removed" << NOCOLOR << " =====" << std::endl;
				} else { 	
					if (pretend)
						implant.pretend();
					else
						implant.perform();
					std::cout << prefix(pretend) << FG_LTGREEN << "Success" << NOCOLOR << " =====" << std::endl;
				}
			} else {
				std::cout << prefix(pretend) << "Installing " << argv[i] << " =====" << std::endl;
				implant.install(argv[i], conflicts, alreadyDone, blocked);
				if (conflicts) {
					std::cout << prefix(pretend) << FG_RED << conflicts << " Conflicts" << NOCOLOR << " =====" << std::endl;
				} else if (blocked) {
					std::cout << prefix(pretend) << FG_RED << "Blocked by .blocked file" << NOCOLOR << " =====" << std::endl;
				} else if (alreadyDone) {
					std::cout << prefix(pretend) << FG_LTGREEN << "Already installed" << NOCOLOR << " =====" << std::endl;
				} else {
					if (pretend)
						implant.pretend();
					else
						implant.perform();
					std::cout << prefix(pretend) << FG_LTGREEN << "Success" << NOCOLOR << " =====" << std::endl;
				}
			}		
		}

		exit(0);
	} catch (std::exception& ex) {
		std::cerr << FG_RED << "ERROR" << NOCOLOR << ": " << ex.what() << std::endl;
		exit(1);
	}
}

