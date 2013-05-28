// -*- C++ -*-

#ifndef _HL_MYLIST_H_
#define _HL_MYLIST_H_

#include <assert.h>
#include <new>

#include "mallocheap.h"

namespace HL {

  template <typename Value,
	    class Allocator = mallocHeap>
  class MyList {
  public:

    MyList (void)
      : _list (NULL),
	_size (0)
    {}

    void push_front (Value v) {
      void * p = getAllocator().malloc (sizeof (ListNode));
      ListNode * n = new (p) ListNode (v, _list);
      _list = n;
      _size++;
    }

    bool pop_front (Value& v) {
      ListNode * n = _list;
      if (n) {
	v = n->value;
	_list = _list->next;
	getAllocator().free ((void *) n);
	_size--;
	return true;
      } else {
	return false;
      }
    }

    int size (void) const {
      return _size;
    }

    void clear (void) {
      Value v;
      while (pop_front (v))
	;
    }

  private:

    class ListNode {
    public:
      ListNode (Value v, ListNode * n)
	: value (v),
	  next (n)
      {}
      Value value;
      ListNode * next;
    };

  public:

    class iterator {
    public:
      iterator (void)
	: _theNode (NULL)
      {}

      Value operator*(void) {
	return _theNode->value;
      }

      iterator (ListNode * l)
	: _theNode (l)
      {}

      bool operator!=(iterator other) const {
	return (this->_theNode != other._theNode);
      }

      bool operator==(iterator other) const {
	return (this->_theNode == other._theNode);
      }

      iterator& operator++ (void) {
	if (_theNode) {
	  _theNode = _theNode->next;
	}
	return *this;
      }

      iterator& operator=(iterator& it) {
	this->_theNode = it->_theNode;
	return *this;
      }
    private:
      ListNode * _theNode;
    };

    iterator begin (void) {
      iterator it (_list);
      return it;
    }

    iterator end (void) {
      iterator it (NULL);
      return it;
    }

  private:

    int _size;
    ListNode * _list;

    Allocator& getAllocator (void) {
      static char buf[sizeof(Allocator)];
      static Allocator * alloc = new (buf) Allocator;
      return *alloc;
    }

  };

}

#endif
