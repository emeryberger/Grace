// -*- C++ -*-

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

#ifndef _XFILE_H_
#define _XFILE_H_

#include <iostream>
#include <list>
#include <semaphore.h>
#include <errno.h>

#include "xatomic.h"
#include "xsection.h"
#include "privateheap.h"

using namespace std;

/// @class xfile
/// @brief Tracks file information, including position and sections.

class xfile {

public:

  typedef std::list<xsection, HL::STLAllocator<xsection, privateheap> >
  sectionType;

#if 0
  xfile (void)
    : _fd (-1),
      _fpos (0),
      _write_end (0),
      _closed (0)
  {
  }
#endif

  xfile (int fd)
    : _fd (fd),
      _fpos (0),
      _write_end (0),
      _closed (false)
  {
  }

  void setClosed (void) { _closed = true; }
  bool isClosed (void) const { return _closed; }

  int getFd (void) const { return _fd; }
  int getFpos (void) const { return _fpos; }

  void incFpos (int off) { _fpos += off; }
  void setFpos (int off) { _fpos = off; }


  void update_fpos (int count)
  {
    _fpos += count;
  }

  void update_write_end (int count)
  {
    _write_end += count;
  }
			
  sectionType::iterator sect_begin()
  {
    _sect = _writesects.begin();
    return _sect;
  }
	
  sectionType::iterator sect_next()
  {
    ++_sect;
    return _sect;
  }

  sectionType::iterator sect_end()
  {
    return _writesects.end();
  }
	
  void list_add (xsection sect)
  {
    _writesects.push_back (sect);
  }
	
  sectionType::iterator list_remove(sectionType::iterator sect)
  {
    return _writesects.erase (_sect);
  }

private:

  /// The file descriptor.
  int  		 _fd;

  /// The position in the file.
  unsigned long  _fpos;

  unsigned long  _write_end;

  /// Has this file been closed?
  bool		 _closed;

  sectionType    _writesects;   /* Sections in the local buffers. */
  sectionType::iterator _sect;

};

#endif /* _XFILE_H_ */
