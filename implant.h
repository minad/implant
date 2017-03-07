#ifndef _implant_h
#define _implant_h

#include "node.h"
#include <set>
#include <vector>

namespace implant {

class Implant {
public:

	Implant(const std::string&, const std::string&, int);
	~Implant();

	void install(const std::string&, int&, bool&, bool&);
	void remove(const std::string&, int&, bool&);
	void perform();
	void pretend();

private:

	Node* installTree(Node*, const std::string&, const std::string&);
	Node* removeTree(Node*, const std::string&, const std::string&, std::string*);
	void log(Node*);
	void cleanup();
	void validate(int&, bool&);

	static bool readSet(const std::string&, std::set<std::string>&);
	static bool readFilteredDir(const std::string&, std::vector<std::string>&);
	
	std::string packageDir;
	std::string targetDir;
	Node*       tree;
	bool        preOrder;
	int         verbose;
};

class Printer {
public:
	Printer(int v)
		: verbose(v) {}
	bool operator()(Node*);
private:
	int verbose;
};

class Performer {
public:
	Performer(int v)
		: verbose(v) {}
	bool operator()(Node*);
private:
	int verbose;
};

struct Validator {
	int  conflicts;
	bool onlyNops;
	Validator()
		: conflicts(0), onlyNops(true) {}
	bool operator()(Node*);
};

}

#endif // _implant_h
