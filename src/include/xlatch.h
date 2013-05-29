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

#ifndef _XLATCH_H_
#define _XLATCH_H_

#include <semaphore.h>

#include "xatomic.h"

/**
 * @class xlatch
 * @brief A cross-process latch (basically a semaphore).
 *
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 */


class xlatch {
public:

  xlatch (int nwaiters = 0)
    : _waiters (nwaiters),
      _unlatched (false)
  {
    sem_init (&_semaphore, true, nwaiters);
  }

  /// @brief Wait for the latch to be released.
  inline void wait (void) {
   if (_unlatched) {
      return;
    } else {
      sem_wait (&_semaphore);
    }
  }

  /// @brief Release the latch, waking up any waiters.
  inline void unlatch (void) {
    _unlatched = true;
    xatomic::memoryBarrier();
    sem_post (&_semaphore);
    sem_post (&_semaphore);
  }


private:

  /// The maximum number of waiters.
  volatile unsigned long _waiters;

  /// True iff the latch has been unlatched (released).
  volatile bool _unlatched;

  /// The underlying semaphore.
  sem_t _semaphore;

};



#endif
