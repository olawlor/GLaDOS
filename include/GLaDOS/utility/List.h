/*
  C++ classes for linked lists.
*/
#ifndef __GLADOS_UTILITY_LIST_H
#define __GLADOS_UTILITY_LIST_H

/**
 Remembers a linked list of NODE objects, by pointer. 
 NODE must contain a NODE * named "next", so "head=head->next;" works.
*/
template <class NODE>
class IntrusiveList {
protected:
	/// This is either the first entry in the list, or 0 if the list is empty.
	NODE *head;
public:
	inline IntrusiveList() { head=0; }
	
	/// Add this node as the beginning of our list
	///  Thread safety: none, cannot be called by multiple threads at once.
	inline void push(NODE *cur) {
		cur->next=head;
		head=cur;
	}
	
	/// Return true if this list contains no nodes.
	inline bool empty(void) const {
		return head==0;
	}
	
	/// If this list is empty, return 0.
	/// Otherwise remove and return the first thing in the list.
	///  Thread safety: none, cannot be called by multiple threads at once.
	inline NODE *pop(void) {
		if (head!=0) {
			NODE *cur=head;
			head=cur->next; // move down the list
			return cur;
		}
		else return 0;
	}
	
	/// The destructor runs delete on everything in the list
	// FIXME: make this selectable via policy, not everything comes from new.
	~IntrusiveList() {
		NODE *cur=head;
		while (cur!=NULL)
		{
			NODE *next=cur->next;
			delete cur;
			cur=next;
		}
	}
private:
	// Don't try to copy or assign this class (messes up head, double free, etc.)
	IntrusiveList(const IntrusiveList &copy) = delete;
	void operator=(const IntrusiveList &copy) = delete;
	// But the destructive rvalue versions are OK (they steal the head pointer)
	//IntrusiveList(IntrusiveList &&src) { head=0; std::swap(head,src.head); }
	//void operator=(IntrusiveList &&src) { std::swap(head,src.head); }
};


#endif

