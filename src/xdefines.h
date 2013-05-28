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

#ifndef _XDEFINES_H_
#define _XDEFINES_H_

#define USE_XLATCH 1

class xdefines {
public:
  enum { STACK_SIZE = 16 * 1024 } ; // 1 * 1048576 };
  enum { STACK_HEAP_SIZE = 1048576UL * 16 };
  enum { HEAP_SIZE = 1048576UL * 512 }; // FIX ME 512 };
  enum { PageSize = 4096UL };
  enum { PAGE_SIZE_MASK = (PageSize-1) };
  enum { NUM_HEAPS = 16 };
};

#endif
