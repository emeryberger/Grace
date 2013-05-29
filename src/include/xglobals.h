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

#ifndef _XGLOBALS_H_
#define _XGLOBALS_H_

#include "xdefines.h"
#include "xpersist.h"

#if defined(__APPLE__)
// We are going to use the Mach-O substitutes for _end, etc.,
// despite the strong admonition not to. Beware.
#include <mach-o/getsect.h>
#endif

#define USE_GRACESTART 1 // FIX ME

extern "C" {

#if !defined(__APPLE__)

#if USE_GRACESTART
  // gracestart is the symbol indicating the start of the global data,
  // but only if the code has been linked with the grace.ld link script.
  extern char gracestart;
#else
  extern char _etext;
  extern char _edata;
#endif

  // _end marks the first address after the UNinitialized global data.
  extern int _end;
#endif

}

// Macros to align to the nearest page down and up, respectively.

#define PAGE_ALIGN_DOWN(x) (((size_t) (x)) & ~xdefines::PAGE_SIZE_MASK)
#define PAGE_ALIGN_UP(x) ((((size_t) (x)) + xdefines::PAGE_SIZE_MASK) & ~xdefines::PAGE_SIZE_MASK)

// Macros that define the start and end addresses of program-wide globals.

#define MAXSIZE 104857600

#if defined(__APPLE__)

#define GLOBALS_START  PAGE_ALIGN_DOWN(((size_t) get_etext() - 1))
#define GLOBALS_END    PAGE_ALIGN_UP(((size_t) get_edata() - 1))

#else

#if USE_GRACESTART
#define GLOBALS_START  PAGE_ALIGN_DOWN(&gracestart)
#else
#define GLOBALS_START  PAGE_ALIGN_DOWN(((size_t) &_edata - 1))
#endif

#define GLOBALS_END    PAGE_ALIGN_UP(((size_t) &_end - 1))

#endif

#define GLOBALS_SIZE   (GLOBALS_END - GLOBALS_START)

#include <iostream>
using namespace std;

/// @class xglobals
/// @brief Maps the globals region onto a persistent store.

#if defined(__APPLE__)

class xglobals {
public:
  xglobals (void) {}
  void commit (void) {}
  void abort (void) {}
  void initialize (void) {}
  void begin (void) {}
  bool nop (void) { return true; }
  void lock (void) {}
  void unlock (void) {}
  bool consistent (void) { return true; }
  bool inRange (void *) { return false; }
  void recordRead (void *) {}
  void recordWrite (void *) {}
  void updateAll (void) {}
};

#else

class xglobals : public xpersist<char,MAXSIZE>  {
public:

  enum { MAX_GLOBALS_SIZE = MAXSIZE };

  xglobals (void)
    : xpersist<char,MAX_GLOBALS_SIZE> ((void *) GLOBALS_START,
				       (size_t) GLOBALS_SIZE)
  {
    //    printf ("gracestart is %p\n", &gracestart);
    //    printf ("globals! start = %p, size = %ld\n", (void *) GLOBALS_START, (size_t) GLOBALS_SIZE);
    // Force assertion even if NDEBUG is on.
#ifdef NDEBUG
#undef NDEBUG
    // Make sure that we have enough room for the globals!
    assert (GLOBALS_SIZE <= MAX_GLOBALS_SIZE);
#define NDEBUG 1
#endif
  }

};
#endif


#endif
