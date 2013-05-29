// -*- C++ -*-

#ifndef _XMEMORY_H_
#define _XMEMORY_H_

#include <signal.h>

#if !defined(_WIN32)
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include <set>

#include "graceheap.h"
#include "xglobals.h"
#include "xrun.h"

// Heap Layers
#include "privateheap.h"
#include "stlallocator.h"


// Encapsulates all memory spaces (globals & heap).

class xmemory {
private:

  // Private on purpose. See getInstance(), below.
  xmemory (void) {
    // Initialize the memory spaces (globals and heap).
    _globals.initialize();
    _heap.initialize();
    // Intercept SEGV signals (used for trapping initial reads and
    // writes to pages).
    installSignalHandler();
  }

public:

  // Just one accessor.  Why? We don't want more than one (singleton)
  // and we want access to it neatly encapsulated here, for use by the
  // signal handler.
  static xmemory& getInstance (void) {
    static char buf[sizeof(xmemory)];
    static xmemory * theOneTrueObject = new (buf) xmemory();
    return *theOneTrueObject;
  }

  void * malloc (size_t sz) {
    void * ptr = _heap.malloc (sz);
    if (ptr) {
      // If we got a valid pointer back from the allocator, write a 0
      // into the first byte. This action ensures that the page
      // containing that address gets written -- and thus, committed.
      // This is probably not necessary but is good to ensure sanity.
      *((char *) ptr) = 0;
    }
    return ptr;
  }

  inline void free (void * ptr) {
    _heap.free (ptr);
  }

  /// @return the allocated size of a dynamically-allocated object.
  inline size_t getSize (void * ptr) {
    // Just pass the pointer along to the heap.
    return _heap.getSize (ptr);
  }

  void begin (void) {
    // Reset pages seen (for signal handler).
    _pages.clear();

    // Reset global and heap protection.
    _globals.begin();
    _heap.begin();
  }

  /// @brief Record a read from this address.
  inline void recordRead (void * addr) {
    if (_heap.inRange (addr)) {
      _heap.recordRead (addr);
      return;
    }
    if (_globals.inRange (addr)) {
      _globals.recordRead (addr);
      return;
    }
    // None of the above - something is wrong.
#if 0
    printf ("out of range!\n");
#endif
  }

  /// @brief Record a write to this address.
  inline void recordWrite (void * addr) {
    if (_heap.inRange (addr)) {
      _heap.recordWrite (addr);
      return;
    }
    if (_globals.inRange (addr)) {
      _globals.recordWrite (addr);
      return;
    }
    // None of the above - something is wrong.
#if 0
    printf ("out of range!\n");
#endif
  }

  /// @brief Add a page to the list of pages seen so far this transaction.
  inline void addPage (void * page) {
    _pages.insert (page);
  }

  /// @return true iff this page has already been touched in this xaction.
  inline bool pageSeen (void * page) {
#if 0
    {
      char buf[255];
      sprintf (buf, "read page %p\n", page);
      printf (buf);
    }
#endif

    pagesetType::iterator i = _pages.find (page);
    bool found = (i != _pages.end());

#if 0
    {
      char buf[255];
      sprintf (buf, "found = %d\n", found);
      printf (buf);
    }
#endif
    
    return found;
  }

  bool isConsistent (void) {
    lock();
    bool result = (_heap.consistent() && _globals.consistent());
    unlock();
    return result;
  }

  bool isNop (void) {
    return (_heap.nop() && _globals.nop());
  }

  void updateAll (void) {
    _heap.updateAll();
    _globals.updateAll();
  }

		
  bool tryToCommit (void) {
    bool committed = false;
    lock();
    if (_heap.consistent() && _globals.consistent()) {
      _heap.commit();
      _globals.commit();
      committed = true;
    }
    unlock();
    return committed;
  }

  void abort (void) {
    _heap.abort();
    _globals.abort();
  }

private:

  void lock (void) {
    _heap.lock();
    _globals.lock();
  }

  void unlock (void) {
    _heap.unlock();
    _globals.unlock();
  }

public:

  /* Signal-related functions for tracking page accesses. */

  /// @brief Signal handler to trap SEGVs.
  static void handle (int signum,
		      siginfo_t * siginfo,
		      void * context) 
  {
    void * addr = siginfo->si_addr; // address of access

    // Check if this was a SEGV that we are supposed to trap.
    if (siginfo->si_code == SEGV_ACCERR) {
      // Compute the page that holds this address.
      void * page = (void *) (((size_t) addr) & ~(xdefines::PageSize-1));

      // Determine whether this access was a read or a write.
      if (!xmemory::getInstance().pageSeen (page)) {
#if 0 // ndef NDEBUG
	printf ("new page: %x\n", page); fflush (stdout);
#endif
	// We haven't seen this page yet, so we consider this a read.
	// Change the page to read-only, and record the read.
	xmemory::getInstance().addPage (page);
	mprotect ((char *) page, 
		  xdefines::PageSize,
		  PROT_READ);
	xmemory::getInstance().recordRead (addr);
      } else {
#if 0 // ndef NDEBUG
	printf ("write to page: %x\n", page); fflush (stdout);
#endif
	// Already saw it: now it was a write.  Give the app full
	// access to the page, and record the write.
	mprotect ((char *) page,
		  xdefines::PageSize,
		  PROT_READ | PROT_WRITE | PROT_EXEC);
	xmemory::getInstance().recordWrite (addr);
      }
    } else if (siginfo->si_code == SEGV_MAPERR) {

#if 1
      printf ("map error!\n");
      char buf[255];
      sprintf (buf, "addr = %p\n", (void *) addr);
      printf ("%s", buf);
      ::abort();
      xmemory::getInstance().abort();
#endif

    } else {

      // Some other error. We assume that it is due to optimistic concurrency,
      // so we re-try.
      printf ("null deref.\n");
      xmemory::getInstance().abort();
      
    }
  }

  /// @brief Install a handler for SEGV signals.
  void installSignalHandler (void) {
    sigset_t block_set;
    sigemptyset (&block_set);
    sigaddset (&block_set, SIGSEGV);
    sigprocmask (SIG_BLOCK, &block_set, NULL);

    // Establish them signal handlers.
#if defined(linux)
    // Set up an alternate signal stack.
    _sigstk.ss_sp = mmap (NULL, SIGSTKSZ, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    _sigstk.ss_size = SIGSTKSZ;
    _sigstk.ss_flags = 0;
    sigaltstack (&_sigstk, (stack_t *) 0);
#endif
    // Now set up a signal handler for SIGSEGV events.
    struct sigaction siga;
    // Get the old handler parameters.
    if (sigaction (SIGSEGV, NULL, &siga) == -1) {
      printf ("fug.\n");
      exit (-1);
    }
    // Point to the handler function.
#if defined(linux)
    siga.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESTART;
#else
    siga.sa_flags = SA_SIGINFO | SA_RESTART;
#endif
    siga.sa_sigaction = xmemory::handle;
    if (sigaction (SIGSEGV, &siga, NULL) == -1) {
      printf ("sfug.\n");
      exit (-1);
    }

    sigprocmask (SIG_UNBLOCK, &block_set, NULL);
  }

private:

  /// The globals region.
  xglobals         	_globals;

  /// The heap used to satisfy all client memory requests.
  graceheap		_heap;

  typedef std::set<void *, less<void *>,
		   HL::STLAllocator<void *, privateheap> > // myHeap> >
  pagesetType;

  /// The set of pages that have been accessed in a given transaction.
  pagesetType      _pages;

  /// A signal stack, for catching signals.
  stack_t          _sigstk;
};

#endif
