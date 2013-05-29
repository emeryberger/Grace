// -*- C++ -*-

/*
  Author: Emery Berger, http://www.cs.umass.edu/~emery
 
  Copyright (c) 2007-8 Emery Berger, University of Massachusetts Amherst.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

/*
 * @file   warpheap.h
 * @brief  A heap optimized to reduce the likelihood of false sharing.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 */


#ifndef _WARPHEAP_H_
#define _WARPHEAP_H_

#include "sassert.h"
#include "xadaptheap.h"


#include "ansiwrapper.h"
#include "kingsleyheap.h"
#include "adaptheap.h"
#include "dllist.h"
#include "sanitycheckheap.h"
#include "zoneheap.h"
#include "ppheap.h"

#define ALIGN_TO_PAGE 1

template <class SourceHeap>
class NewSourceHeap : public SourceHeap {
public:
  void * malloc (size_t sz) {

#if 0
    char buf[255];
    sprintf (buf, "source malloc %d\n", sz);
    printf (buf);
#endif

#if ALIGN_TO_PAGE
    if (sz >= 4096 - sizeof(objectHeader)) {
      sz += 4096;
    }
#endif
    void * ptr = SourceHeap::malloc (sz + sizeof(objectHeader));
    if (!ptr) {
      return NULL;
    }
#if ALIGN_TO_PAGE
    if (sz >= 4096 - sizeof(objectHeader)) {
      ptr = (void *) (((((size_t) ptr) + 4095) & ~4095) - sizeof(objectHeader));
    }
#endif
    objectHeader * o = new (ptr) objectHeader (sz);
    void * newptr = getPointer(o);
    assert (getSize(newptr) >= sz);
    return newptr;
  }
  void free (void * ptr) {
    SourceHeap::free ((void *) getObject(ptr));
  }
  size_t getSize (void * ptr) {
    //    printf ("ptr = %p\n", ptr);
    objectHeader * o = getObject(ptr);
    size_t sz = o->getSize();
    if (sz == 0) {
      printf ("error!\n");
    }
    return sz;
  }
private:

  class objectHeader;

  static objectHeader * getObject (void * ptr) {
    objectHeader * o = (objectHeader *) ptr;
    return (o - 1);
  }

  static void * getPointer (objectHeader * o) {
    return (void *) (o + 1);
  }

  class objectHeader {
  public:
    enum { MAGIC = 0xCAFEBABE };
    objectHeader (size_t sz)
      : _size (sz),
	_magic (MAGIC)
    {}
    size_t getSize (void) { sanityCheck(); return _size; }
  private:
    void sanityCheck (void) {
      if (_magic != MAGIC) {
	fprintf (stderr, "HLY FK.\n");
	::abort();
      }
    }
    size_t _size;
    size_t _magic;
  };
};


enum { CHUNKY = 1048576 };

template <class SourceHeap>
class KingsleyStyleHeap :
  public 
  HL::ANSIWrapper<
  HL::StrictSegHeap<Kingsley::NUMBINS,
		    Kingsley::size2Class,
		    Kingsley::class2Size,
		    HL::AdaptHeap<HL::DLList, NewSourceHeap<SourceHeap> >,
		    NewSourceHeap<HL::ZoneHeap<SourceHeap, CHUNKY> > > >
{
private:

  typedef 
  HL::ANSIWrapper<
  HL::StrictSegHeap<Kingsley::NUMBINS,
		    Kingsley::size2Class,
		    Kingsley::class2Size,
		    HL::AdaptHeap<HL::DLList, NewSourceHeap<SourceHeap> >,
		    NewSourceHeap<HL::ZoneHeap<SourceHeap, CHUNKY> > > >
  SuperHeap;

public:
  KingsleyStyleHeap (void) {
    //    printf ("this kingsley = %p\n", this);
  }

  void * malloc (size_t sz) {
    void * ptr = SuperHeap::malloc (sz);
    //    printf ("malloc(%d) ptr = %p\n", sz, ptr);
    return ptr;
  }

private:
  char buf[4096 - sizeof(SuperHeap)];
};



template <class SourceHeap>
class PerThreadHeap : public PPHeap<xdefines::NUM_HEAPS, KingsleyStyleHeap<SourceHeap> > {};


template <int NumHeaps,
	  class SourceHeap>
class warpheap :
  public xadaptheap<PerThreadHeap, SourceHeap> {};


#endif // _WARPHEAP_H_

