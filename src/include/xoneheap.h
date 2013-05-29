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

#ifndef _XONEHEAP_H_
#define _XONEHEAP_H_

/**
 * @class xoneheap
 * @brief Wraps a single heap instance.
 *
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 */

template <class SourceHeap>
class xoneheap {
public:

  void initialize (void) { getHeap()->initialize(); }
  void begin (void) { getHeap()->begin(); }
  void abort (void) { getHeap()->abort(); }
  bool consistent (void) { return getHeap()->consistent(); }
  void commit (void) { getHeap()->commit(); }
  void updateAll (void) { getHeap()->updateAll(); }
  void commitMemory (void) { getHeap()->commitMemory(); }

  void stats (void) { getHeap()->stats(); }

  void lock (void) { getHeap()->lock(); }
  void unlock (void) { getHeap()->unlock(); }

  bool nop (void) { return getHeap()->nop(); }
  bool inRange (void * ptr) { return getHeap()->inRange(ptr); }
  void recordWrite (void * ptr) { getHeap()->recordWrite(ptr); }
  void recordRead (void * ptr) { getHeap()->recordRead(ptr); }


  void * malloc (size_t sz) { return getHeap()->malloc(sz); }
  void free (void * ptr) { getHeap()->free(ptr); }
  size_t getSize (void * ptr) { return getHeap()->getSize(ptr); }

private:

  SourceHeap * getHeap (void) {
    static char heapbuf[sizeof(SourceHeap)];
    static SourceHeap * _heap = new (heapbuf) SourceHeap;
#if 0
    char buf[255];
    sprintf (buf, "heap. %p\n", _heap);
    printf (buf);
#endif
    return _heap;
  }

};


#endif // _XONEHEAP_H_
