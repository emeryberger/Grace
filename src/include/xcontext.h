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

#ifndef _XCONTEXT_H_
#define _XCONTEXT_H_

#include <signal.h>
#include <stdio.h>

#if defined(__APPLE__)
#define _XOPEN_SOURCE // required to use getcontext and friends
#endif

#include <ucontext.h>

/**
 * @class xcontext
 * @brief Lets a program rollback to a previous execution context.
 *
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @note  Adapted from code by Dan Piponi <http://homepage.mac.com/sigfpe/Computing/continuations.html>
 */

#if defined(__GNUC__)
#define NO_INLINE       __attribute__ ((noinline))
#elif defined(_WIN32)
#define NO_INLINE __declspec(noinline)
#endif

class xcontext {
public:


  /// @brief Save current calling context (i.e., current continuation).
  NO_INLINE void commit (void);

  /// @brief Restore the previously saved context.
  NO_INLINE void abort (void);

  /// @brief Initialize this with the highest pointer possible on the stack.
  NO_INLINE void initialize (void);

private:

  static void * stacktop (void);

  /// A pointer to the base (highest address) of the stack.
  unsigned long * _pbos;

  /// How big can the stack be (in words).
  enum { MAX_STACK_SIZE = 1048576 };

  /// The saved registers, etc.
  ucontext_t _registers;

  /// Current saved stack size.
  int _stackSize;

  /// The saved stack contents.
  unsigned long _stack[MAX_STACK_SIZE];

  NO_INLINE void save_stack (unsigned long *pbos, unsigned long *ptos);

  NO_INLINE void getContext (void);

  NO_INLINE void restoreStack (int once_more);

};


#endif
