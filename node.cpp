#include "node.h"
#include "color.h"
#include <iostream>

namespace implant {

using std::string;
using std::endl;
using std::cout;

Node::Node(Node* parent, const string& s, const string& t)
	: parent(parent), source(s), target(t), action(NOP) {
}

Node::~Node() {
	for (std::list<Node*>::iterator i = child.begin(); i != child.end(); ++i)
		delete *i;
}

void Node::add(Node* node) {
	child.push_back(node);
}

void Node::print(int verbose) {
	switch (action) {
	case NOP:
		if (verbose >= 2)
			cout << '[' << FG_LTWHITE << "NOP" << NOCOLOR << "]      " << target << (msg == "" ? "" : " - ") << msg  << endl;
		break;
	case CONFLICT:
			cout << '[' << FG_RED     << "CONFLICT" << NOCOLOR << "] " << target << (msg == "" ? "" : " - ") << msg  << endl;
		break;
	case UNFOLD:
		if (verbose >= 1)
			cout << '[' << FG_LTBLUE  << "UNFOLD" << NOCOLOR << "]   " << target << (msg == "" ? "" : " - ") << msg  << endl;
		break;
	case LINK:
		if (verbose >= 1)
			cout << '[' << FG_LTGREEN << "LINK" << NOCOLOR << "]     " << target << (msg == "" ? "" : " - ") << msg  << endl;
		break;
	case MKDIR:
		if (verbose >= 1)
			cout << '[' << FG_LTGREEN << "MKDIR" << NOCOLOR << "]    " << target << (msg == "" ? "" : " - ") << msg  << endl;
		break;
	case REMOVE:
		if (verbose >= 1)
			cout << '[' << FG_LTGREEN << "REMOVE" << NOCOLOR << "]   " << target << (msg == "" ? "" : " - ") << msg  << endl;
		break;
	case FOLD:
		if (verbose >= 1)
			cout << '[' << FG_LTBLUE  << "FOLD" << NOCOLOR << "]     " << target << (msg == "" ? "" : " - ") << msg  << endl;
		break;
	}
}
	
}
