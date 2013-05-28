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
 * @file   xrun.h
 * @brief  The main engine for consistency management, etc.
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 */

#ifndef _XRUN_H_
#define _XRUN_H_

#include "xdefines.h"

// threads
#include "xthread.h"

// memory
#include "xmemory.h"

// calling context
#include "xcontext.h"

// Heap Layers
#include "util/sassert.h"

// I/O
#include "xio.h"

// Grace utilities
#include "xatomic.h"


class xrun {

private:

  xrun (void)
    : _isInitialized (false),
      _pred (0),
      _memory (xmemory::getInstance())
  {
  }

public:

  static xrun& getInstance (void) {
    static char buf[sizeof(xrun)];
    static xrun * theOneTrueObject = new (buf) xrun();
    return *theOneTrueObject;
  }

  /// @brief Initialize the system.
  void initialize (void)
  {
    if (!_isInitialized) {
      _isInitialized = true;

      // Initialize the context.
      _context.initialize();
      
      // Set the current _tid to our process id.
      _thread.setId (getpid());

      // Set thread to spawn no more threads than number of processors.
      _thread.setMaxThreads (HL::CPUInfo::getNumProcessors());
      
      // Make sure the heap gets initialized.
      atomicBegin();
      malloc(1);
      atomicEnd();

      // Now we start our first real transaction (in main).
      atomicBegin();
    } else {
      printf ("OH NOES\n");
      ::abort();
      // We should only initialize ONCE.
    }
  }


  void finalize (void)
  {
    // If the tid was set, it means that this instance was
    // initialized: end the transaction (at the end of main()).
    if (_thread.getId()) {
      atomicEnd();
    }
    // Wait for logical predecessor, if any.
    waitPred();
  }

  /* Transaction-related functions. */

  void punctuate (void) {
    atomicEnd();
    atomicBegin();
  }

  /* Thread-related functions. */

  /// @return the "thread" id.
  inline int id (void) const {
    return _thread.getId();
  }


  /// @brief Spawn a thread.
  /// @return an opaque object used by sync.

  inline void * spawn (threadFunction * fn,
		       void * arg)
  {
    return _thread.spawn (this, fn, arg);
  }

  /// @brief Wait for a thread.
  inline void sync (void * v, void ** result) {
    _thread.sync (this, v, result);
  }

  /* Heap-related functions. */

  inline void * malloc (size_t sz) {
    return _memory.malloc (sz);
  }

  inline void free (void * ptr) {
    _memory.free (ptr);
  }

  inline size_t getSize (void * ptr) {
    return _memory.getSize (ptr);
  }

  //// buffered I/O functions

  /// @brief Open a file.
  int open (char * pathname, int flags, mode_t mode)
  {
    ensureIrrevocable();
    return _xio.open(pathname, flags, mode);
  }

  /// @brief Close a file.
  int close (int fd)
  {
    ensureIrrevocable();
    return _xio.close(fd);
  }
  
  /// @brief Write data to a file (buffered until the transaction ends).
  int write (int fd, void * buf, int count)
  {
    ensureIrrevocable();
    return _xio.write (fd, buf, count);
  }
  
  ///// irrevocable I/O operations.

  /// @brief Read data from a file.
  int read (int fd, void * buf, int count)
  {
    ensureIrrevocable();
    return _xio.read (fd, buf, count);
  }
  
  int lseek (int fd, int offset, int whence)
  {
    ensureIrrevocable();
    return _xio.lseek (fd, offset, whence);
  }

  int connect (int fd, struct sockaddr *serv_addr, socklen_t addrlen)
  {
    ensureIrrevocable();
    return _xio.connect (fd, serv_addr, addrlen);
  }

  int accept (int fd, struct sockaddr *addr, socklen_t * addrlen)
  {
    ensureIrrevocable();
    return _xio.accept(fd, addr, addrlen);
  }

  /// TO DO - need to add socket() here as well.

  /// @brief Start a transaction.
  void atomicBegin (void) {
    // Roll back to here on abort.
    _context.commit();

    // Now start.
    _memory.begin();
  }

  /// @brief End a transaction, aborting it if necessary.
  void atomicEnd (void) {

    // First, attempt to commit.
    commitResult r = atomicCommit();
    
    // Now decide what to do, depending on the result of the
    // attempted commit.
    switch (r) {
    case FAILED:
      // We failed to commit, so we need to roll back this thread.
      abort();
      // We will never get here.
      assert(0);
      break;
    case SUCCEEDED:
      // We successfully committed (without the NULL
      // optimization). Clear the predecessor so the main thread
      // will no longer wait for anyone (since it has already waited
      // for its predecessor).
      _pred = 0;
      break;
    case OPTIMIZED:
      // If the commit was optimized, then we need to maintain the
      // previous predecessor (since we never waited for it) --
      // which we do by doing nothing (i.e., by NOT clearing the
      // predecessor).
      break;
    };
  }

  inline void setPred (int tid) {
    _pred = tid;
  }

  inline void waitPred (void) {
    if (!_pred) {
      return;
    }
    _thread.waitExited (_pred);
  }


private:


  void ensureIrrevocable (void) {
    // First, wait for our immediate predecessor to complete.  NOTE
    // that this does NOT take advantage of the possibility of the
    // NULL optimization (see atomicCommit()).
    waitPred();

    // Now we check our state. If it is not consistent, we have to roll back
    // and re-execute.
    if (!_memory.isConsistent()) {
      abort();
    }
  }


  typedef enum { FAILED, SUCCEEDED, OPTIMIZED } commitResult;


  /// @brief Check consistency and commit atomically (if consistent).
  inline commitResult atomicCommit (void) {

    // First, the NULL optimization.  If we haven't read or written
    // anything, we don't have to wait or commit --- just update our
    // view of memory and return that we executed an OPTIMIZED commit.

    if (_memory.isNop()) {
      _memory.updateAll();
      return OPTIMIZED;
    }

    // Wait for our immediate predecessor to complete.
    waitPred();

    // Now we try to commit our state. Iff we succeeded, we return true.

    bool committed = _memory.tryToCommit();
    if (committed) {
      _xio.commit();
      fflush (stdout);
      return SUCCEEDED;
    } else {
      return FAILED;
    }
  }

  /// @brief Abort a transaction in progress.
  void abort (void) {
#if 0 // ndef NDEBUG
    printf ("ROLLBACK from thread %d\n", id());
#endif
    _memory.abort();
    _context.abort();
    // We should never get here.
    assert (0);
  }

  xthread	   _thread;

  /// The saved context of the current xaction, to support rollbacks.
  xcontext         _context;

  /// The memory manager (for both heap and globals).
  xmemory& 	   _memory;

  volatile  bool   _isInitialized;

  /// The I/O manager.
  xio		   _xio;

  /// The last transaction we are waiting for.
  int 		   _pred;

};


#endif
