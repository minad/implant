#ifndef _node_h
#define _node_h

#include <string>
#include <list>

namespace implant {

enum Action {
	// === General ===
	NOP,			// No operation
	CONFLICT,		// Conflict
	// === Installation ===
	UNFOLD,			// Replace link with directory
	LINK,			// Create new link, nothing exists yet
	MKDIR,			// New directory, nothing exists yet
	// === Remove ===
	REMOVE,			// Remove existing link
	FOLD			// Replace directory with link
};
	
class Node {
public:

	Node(Node*, const std::string&, const std::string&);
	~Node();

	Node* getParent() const;

	const std::string& getSource() const;
	const std::string& getTarget() const;

	void setAction(Action, const std::string&);
	Action getAction() const;
	const std::string& getMessage() const;

	void add(Node*);

	template<class Walker>
	void preOrder(Walker&);

	template<class Walker>
	void postOrder(Walker&);

	void print(int);

private:
	Node*		 parent;
	std::string	 source;
	std::string	 target;
	std::list<Node*> child;
	Action		 action;
	std::string      msg;
};

inline Node* Node::getParent() const {
	return parent;
}

inline const std::string& Node::getSource() const {
	return source;
}

inline const std::string& Node::getTarget() const {
	return target;
}

inline void Node::setAction(Action a, const std::string& m) {
	action = a;
	msg = m;
}

inline Action Node::getAction() const {
	return action;
}

inline const std::string& Node::getMessage() const {
	return msg;
}

template<class Walker>
void Node::preOrder(Walker& w) {
	if (w(this)) {
		for (std::list<Node*>::const_iterator i = child.begin(); i != child.end(); ++i)
			(*i)->preOrder(w);
	}
}

template<class Walker>
void Node::postOrder(Walker& w) {
	for (std::list<Node*>::const_iterator i = child.begin(); i != child.end(); ++i)
		(*i)->postOrder(w);
	w(this);
}

}

#endif // _node_h
