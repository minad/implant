#include "implant.h"
#include "util.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <iostream>

namespace implant {

using std::string;
using std::set;
using std::vector;
using std::invalid_argument;
using std::runtime_error;
using namespace util;

const char* EXCLUDES_FILE = ".excludes";
const char* INCLUDES_FILE = ".includes";
const char* IGNORE_FILE   = ".ignore";
const char* BLOCK_FILE    = ".blocked";

Implant::Implant(const string& p, const string& t, int v)
	: packageDir(collapse(p)), targetDir(collapse(t)), tree(0), verbose(v) {
	if (verbose >= 2) {
		std::cout << "Package directory: " << packageDir
			  << "\nTarget directory:  " << targetDir << std::endl;
	}
	if (packageDir[0] != '/')
		throw invalid_argument("Package directory path is not absolute");
	if (targetDir[0] != '/')
		throw invalid_argument("Target directory path is not absolute");
	if (targetDir == packageDir)
		throw invalid_argument("Target and package directory must not be the same");
}

Implant::~Implant() {
	cleanup();
}

void Implant::install(const string& pkg, int& conflicts, bool& alreadyDone, bool& blocked) {
	if (pkg.find('/') != string::npos)
		throw invalid_argument("Package name must not contain /");
	cleanup();
	struct stat st;
	blocked = stat((packageDir / pkg / BLOCK_FILE).c_str(), &st) == 0;
	if (!blocked) {
		tree = installTree(0, packageDir / pkg, targetDir);
		preOrder = true;
		validate(conflicts, alreadyDone);
	} else {
		conflicts = 0;
		alreadyDone = false;
	}
}

void Implant::remove(const string& pkg, int& conflicts, bool& alreadyDone) {
	if (pkg.find('/') != string::npos)
		throw invalid_argument("Package name must not contain /");
	cleanup();
	tree = removeTree(0, packageDir / pkg, targetDir, 0);
	preOrder = false;
	validate(conflicts, alreadyDone);
}

void Implant::perform() {
	if (!tree)
		throw runtime_error("No action tree");
	Performer performer(verbose);
	if (preOrder)
		tree->preOrder(performer);
	else
		tree->postOrder(performer);
	delete tree;
	tree = 0;
}

void Implant::pretend() {
	if (!tree)
		throw runtime_error("No action tree");
	Printer printer(verbose);
	if (preOrder)
		tree->preOrder(printer);
	else
		tree->postOrder(printer);
	delete tree;
	tree = 0;
}

void Implant::validate(int& conflicts, bool& alreadyDone) {
	Validator validator;
	tree->preOrder(validator);
	conflicts = validator.conflicts;
	alreadyDone = validator.onlyNops;
	if (alreadyDone || conflicts)
		cleanup();
}

Node* Implant::installTree(Node* parent, const string& src, const string& tgt) {
	Node* node = new Node(parent, src, tgt);
	try {
		// Check if file exists
		struct stat st;
		if (stat(src.c_str(), &st) < 0)
			throw SystemError("Stat on " + src + " not found");

		// Source is no directory (package)
		if (!S_ISDIR(st.st_mode)) {
			// Target not there?
			if (lstat(tgt.c_str(), &st) < 0) {
				node->setAction(LINK, "");
			} else if (S_ISLNK(st.st_mode)) {
				if (stat(tgt.c_str(), &st) < 0) {
					node->setAction(CONFLICT, "Dead link found");
				} else if (!S_ISDIR(st.st_mode)) {
					string link = w_readlink(tgt);
					if (src == link)
						node->setAction(NOP, "Exists already");
					else
						node->setAction(CONFLICT, "Link somewhere else");
				} else {
					node->setAction(CONFLICT, "Link to directory found");
				}
			} else {
				node->setAction(CONFLICT, "File/Directory found");
			}
			
			return node;
		}

		// File in package is directory
		// Target not found?
		if (lstat(tgt.c_str(), &st) < 0) {
			node->setAction(LINK, "");
		} else if (S_ISLNK(st.st_mode)) {
			if (stat(tgt.c_str(), &st) < 0) {
				node->setAction(CONFLICT, "Dead link found");
				return node;
			}
			
			if (!S_ISDIR(st.st_mode)) {
				node->setAction(CONFLICT, "Link to file found");
				return node;
			}

			string link = w_readlink(tgt);
			if (src == link) {
				node->setAction(NOP, "Exists already");
				return node;
			} else if (link.find(packageDir + "/", 0) == 0) {
				node->setAction(UNFOLD, "Link used by another package");
			} else {
				node->setAction(NOP, "Exists already (Link)"); // Mein LFS Beispiel: Link von /usr/bin -> /bin
			}
		} else if (S_ISDIR(st.st_mode)) {
			if (parent && parent->getAction() == UNFOLD)
				node->setAction(UNFOLD, "Link used by another package");
			else 
				node->setAction(NOP, "Exists already");
		} else {
			node->setAction(CONFLICT, "File found");
			return node;
		}

		vector<string> child;
		bool filesExcluded  = readFilteredDir(src, child);
		for (size_t i = 0; i < child.size(); ++i)
			node->add(installTree(node, src / child[i], tgt / child[i]));

		// Parent directories can not be links anymore,
		// because this directory excluded something
		if (filesExcluded) {
			for (Node* n = node; n; n = n->getParent()) {
				if (n->getAction() == LINK)
					n->setAction(MKDIR, "Hidden children");	
			}
		}

		return node;
	} catch (...) {
		delete node;
		throw;
	}
}

Node* Implant::removeTree(Node* parent, const string& src, const string& tgt, string* foldPackage) {
	Node* node = new Node(parent, src, tgt);
	try {
		// Check if file exists
		struct stat st;
		if (stat(src.c_str(), &st) < 0)
			throw SystemError("Stat on " + src + " not found");

		// File in package is no directory
		if (!S_ISDIR(st.st_mode)) {
			// Target not there?
			if (lstat(tgt.c_str(), &st) < 0) {
				node->setAction(NOP, "Already removed");
			} else if (S_ISLNK(st.st_mode)) {
				if (stat(tgt.c_str(), &st) < 0) {
					node->setAction(CONFLICT, "Dead link found");
				} else if (!S_ISDIR(st.st_mode)) {
					string link = w_readlink(tgt);
					if (src == link)
						node->setAction(REMOVE, "");
					else
						node->setAction(CONFLICT, "Link not owned by package");
				} else {
					node->setAction(CONFLICT, "Link to directory found");
				}
			} else {
				node->setAction(CONFLICT, "File/Directory found");
			}
			return node;
		}

		// File in package is directory
		// Target not found?
		if (lstat(tgt.c_str(), &st) < 0) {
			node->setAction(NOP, "Already removed");
			return node;
		}
		
		if (S_ISLNK(st.st_mode)) {
			if (stat(tgt.c_str(), &st) < 0) {
				node->setAction(CONFLICT, "Dead link found");
			} else if (S_ISDIR(st.st_mode)) {
				string link = w_readlink(tgt);
				if (src == link) {
					node->setAction(REMOVE, "");
				} else if (link.find(packageDir + "/", 0) == 0) {
					node->setAction(NOP, "Owned by another package");
				} else {
					node->setAction(NOP, "Not owned by package");
					vector<string> child;
					readFilteredDir(src, child);
					for (size_t i = 0; i < child.size(); ++i)
						node->add(removeTree(node, src / child[i], tgt / child[i], 0));
				}
			} else {
				node->setAction(CONFLICT, "Link to file found");
			}
		} else if (!S_ISDIR(st.st_mode)) {
			node->setAction(CONFLICT, "File found");
		} else {
			vector<string> tgtChild, srcChild;
			w_readdir(tgt, tgtChild);
			readFilteredDir(src, srcChild);
			
			string package;
			bool packageMatches = true;
			size_t removed = 0;
			for (size_t i = 0; i < tgtChild.size(); ++i) {
				vector<string>::iterator j = std::find(srcChild.begin(), srcChild.end(), tgtChild[i]);

				if (j != srcChild.end()) {
					std::string childPackage;
					Node* child = removeTree(node, src / tgtChild[i], tgt / tgtChild[i], &childPackage);
					node->add(child);
					if (child->getAction() == REMOVE) {
						++removed;
					// Check if package matches
					} else if (packageMatches && child->getAction() == FOLD) {
						if (package != "") {
							if (childPackage.find(package + "/", 0) != 0)
								packageMatches = false;
						} else {
							package = dirname(childPackage);
						}
					} else {
						packageMatches = false;
					}
				// File from other package
				// Check if it matches one single package
				} else if (packageMatches) {
					// No a link
					lstat((tgt / tgtChild[i]).c_str(), &st);
					if (!S_ISLNK(st.st_mode)) {
						packageMatches = false;
						continue;
					}

					// Link outside package directory
					string link = w_readlink(tgt / tgtChild[i]);
					if (link.find(packageDir + "/", 0) != 0) {
						packageMatches = false;
						continue;
					}
					
					if (package != "") {
						// Different packages
						if (link.find(package + "/", 0) != 0)
							packageMatches = false;
						continue;
					}
					package = dirname(link);
				}
			}

			if (removed == tgtChild.size()) {
				node->setAction(REMOVE, "Directory empty");
			} else if (packageMatches) {
				vector<string> crap;
				if (readFilteredDir(package, crap)) {
					node->setAction(NOP, "Owned by another package (Excluded)");
				} else {
					node->setAction(FOLD, "Owned by another package (Nothing Excluded)");
					if (foldPackage)
						*foldPackage = package;
				}
			} else {
				node->setAction(NOP, "Not empty");
			}
		}

		return node;
	} catch (...) {
		delete node;
		throw;
	}
}

bool Performer::operator()(Node* node) {
	node->print(verbose);
	switch (node->getAction()) {
	case NOP:
		return true; // perform children
	case CONFLICT:
		throw runtime_error("Performing tree with conflict");
	case UNFOLD: {
			string link = w_readlink(node->getTarget());
			vector<string> child;
			w_readdir(node->getTarget(), child);
			w_unlink(node->getTarget());
			
			struct stat st;
			if (stat(node->getSource().c_str(), &st) < 0)
				throw SystemError("Stat on " + node->getSource() + " failed");
			w_mkdir(node->getTarget(), st.st_mode);
			w_chown(node->getTarget(), st.st_uid, st.st_gid);

			for (size_t i = 0; i < child.size(); ++i)
				w_symlink(link / child[i], node->getTarget() / child[i]);
			return true; // perform children
		}
	case LINK:
		w_symlink(node->getSource(), node->getTarget());
		return false; // do not perform children
	case MKDIR: {
			struct stat st;
			if (stat(node->getSource().c_str(), &st) < 0)
				throw SystemError("Stat on " + node->getSource() + " failed");
			w_mkdir(node->getTarget(), st.st_mode);
			w_chown(node->getTarget(), st.st_uid, st.st_gid);
			return true; // perform children
		}
	case REMOVE:
		w_remove(node->getTarget());
		return false; // there are no childrens anymore
	case FOLD: {
			vector<string> child;
			w_readdir(node->getTarget(), child);
			string link = dirname(w_readlink(node->getTarget() / child[0]));
			
			for (size_t i = 0; i < child.size(); ++i)
				w_unlink(node->getTarget() / child[i]);

			w_rmdir(node->getTarget());
			w_symlink(link, node->getTarget());
			return false; // there are no childrens anymore
		}
	default:
		throw runtime_error("Unknown action");
	}
}

bool Validator::operator()(Node* node) {
	if (node->getAction() != NOP)
		onlyNops = false;
	if (node->getAction() == CONFLICT) {
		node->print(0); // conflict is printed ever
		++conflicts;
		return false;
	}
	return true;
}

bool Printer::operator()(Node* node) {
	node->print(verbose);
	switch (node->getAction()) {
	case NOP:
	case CONFLICT:
	case UNFOLD:
	case MKDIR:
		return true; // perform children
	case LINK:
	case REMOVE:
	case FOLD:	
		return false; // there are no childrens anymore
	default:
		throw runtime_error("Unknown action");
	}
}

void Implant::cleanup() {
	if (tree) {
		delete tree;
		tree = 0;
	}
}

bool Implant::readSet(const string& file, set<string>& s) {
	std::ifstream in(file.c_str());
	if (!in.good())
		return false;
	char buffer[255];
	while (!in.eof()) {
		in.getline(buffer, sizeof(buffer));
		
		for (char* p = buffer; *p; ++p) {
			if (*p == '#') {
				*p = 0;
				break;
			}
		}

		char* begin = buffer;
		while (*begin && isspace(*begin))
			++begin;
		
		char* end = buffer + strlen(buffer);
		while (end > begin) {
			if (!isspace(*(end - 1))) {
				*end = 0;
				break;
			}
			--end;
		}
		
		if (begin < end)
			s.insert(begin);
	}
	return true;
}

bool Implant::readFilteredDir(const string& dir, vector<string>& result) {
	std::set<string> excludes, includes;
	bool excludesFound = readSet(dir / EXCLUDES_FILE, excludes);
	bool includesFound = readSet(dir / INCLUDES_FILE, includes);
	bool filesExcluded = excludesFound || includesFound;
	
	std::vector<string> child;
	w_readdir(dir, child);
	for (size_t i = 0; i < child.size(); ++i) {
		if (child[i] == INCLUDES_FILE || child[i] == EXCLUDES_FILE || child[i] == IGNORE_FILE)
			continue;
		
		struct stat st;
		if (excludesFound && excludes.find(child[i]) != excludes.end() ||
		    includesFound && includes.find(child[i]) == includes.end() ||
		    stat((dir / child[i] / IGNORE_FILE).c_str(), &st) == 0) {
			filesExcluded = true;
			continue;
		}

		result.push_back(child[i]);
	}
	return filesExcluded;
}

}

