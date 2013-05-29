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

#ifndef _XADAPTHEAP_H_
#define _XADAPTHEAP_H_

/**
 * @class xadaptheap
 * @brief Manages a heap whose metadata is allocated from a given source.
 *
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 */

template <template <class S> class Heap,
	  class Source>
class xadaptheap : public Source {
public:

  xadaptheap (void)
  {
    // Instantiate the heap in memory obtained from the source.
    void * buf = Source::malloc (sizeof(Heap<Source>));
    _heap = new (buf) Heap<Source>;
  }

  virtual ~xadaptheap (void) {
    Source::free (_heap);
  }

  void * malloc (size_t sz) {
    return _heap->malloc (sz);
  }

  void free (void * ptr) {
    _heap->free (ptr);
  }

  size_t getSize (void * ptr) {
    return _heap->getSize (ptr);
  }

private:

  Heap<Source> * _heap;

};


#endif // _XADAPTHEAP_H_

