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

/**
 * @file   bumpheap.h
 * @brief  A basic bump pointer heap, used as a source for other heap layers.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 */


#ifndef _BUMPHEAP_H_
#define _BUMPHEAP_H_

#include <stdlib.h>

template <int Size, class Source>
class bumpheap : public Source
{
  typedef Source parent;

public:

  bumpheap (void):
    _start ((char *) Source::malloc (Size)),
    _position (_start),
    _remaining (Size),
    _magic (0xCAFEBABE)
  {}

  inline void * malloc (size_t sz) {
    sanityCheck();

    // First, round up the size to a double, if it wasn't that big already.
    sz = (sz < sizeof(double)) ? sizeof(double) : sz;
    // Now, round up all size requests to the nearest double.
    sz = (sz + sizeof(double) - 1) & ~(sizeof(double) - 1);

    // Return NULL if we are out of memory.
    if (_remaining < sz) {
      return NULL;
    }

    // Increment the bump pointer and drop the amount of memory.
    void * p = _position;
    _remaining -= sz;
    _position += sz;

    sanityCheck();

    return p;
  }

  // These should never be used.
  inline void free (void *) { sanityCheck(); }
  inline size_t getSize (void *) { sanityCheck(); return 0; } // FIX ME

private:

  void sanityCheck (void) {
#if 1
    if (_magic != 0xCAFEBABE) {
      printf ("WTF!\n");
      ::abort();
    }
#endif
  }


  /// The start of the heap area.
  char *  _start;

  /// The current bump pointer.
  char *  _position;

  /// The amount of memory remaining.
  size_t  _remaining;

  /// A magic number for sanity checking.
  size_t  _magic;

};

#endif
