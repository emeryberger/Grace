// -*- C++ -*-

#ifndef _XPERSIST_H_
#define _XPERSIST_H_

#include <set>

#if !defined(_WIN32)
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xatomic.h"
#include "ansiwrapper.h"
#include "freelistheap.h"
#include "zoneheap.h"

#include "stlallocator.h"
#include "privateheap.h"
#include "xplock.h"
#include "xdefines.h"

#define USE_MSYNC 0

#if defined(sun)
extern "C" int madvise(caddr_t addr, size_t len, int advice);
#endif

/**
 * @class xpersist
 * @brief Makes a range of memory persistent and consistent.
 *
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 */

template <class Type,
	  int NElts = 1>
class xpersist {
public:

  /// @arg startaddr  the optional starting address of the local memory.
  xpersist (void * startaddr = 0, 
	    size_t startsize = 0)
    : _isLocked (false),
      _startaddr (startaddr),
      _startsize (startsize),
      _initialized (false)
  {

    char _backingFname[L_tmpnam];
    char _versionsFname[L_tmpnam];

    // Get a temporary file name (which had better not be NFS-mounted...).
    sprintf (_backingFname, "graceMXXXXXX");
    _backingFd = mkstemp (_backingFname);

    if ((_backingFd == -1) || (_versionsFd == -1)) {
      fprintf (stderr, "Failed to make persistent file.\n");
      ::abort();
    }

    // Get another temporary file name (which had better not be NFS-mounted...).
    sprintf (_versionsFname, "graceVXXXXXX");
    _versionsFd = mkstemp (_versionsFname);

    // Set the files to the sizes of the desired object.
    int result;
    result = ftruncate (_backingFd,  NElts * sizeof(Type));
    result |= ftruncate (_versionsFd, VersionArrayLength * sizeof(int));
    if (result) {
      // Some sort of mysterious error.
      // Adios.
      fprintf (stderr, "Mysterious error with ftruncate.\n");
      ::abort();
    }

    // Get rid of the files when we exit.
    unlink (_backingFname);
    unlink (_versionsFname);

    //
    // Establish two maps to the backing file.
    //

    // The persistent map is shared.
    _persistentMemory = (Type *) mmap (NULL,
				       NElts * sizeof(Type),
				       PROT_READ | PROT_WRITE,
				       MAP_SHARED,
				       _backingFd,
				       0);

    // If we specified a start address, copy the contents into the
    // persistent area now because the transient memory map is going
    // to squash it.
    if (_startaddr) {
      memcpy (_persistentMemory, _startaddr, _startsize);
    }

    // The transient map is optionally fixed at the desired start
    // address. We will change the mapping to private later before we
    // use it.
    _transientMemory = (Type *) mmap (_startaddr,
				      NElts * sizeof(Type),
				      PROT_READ | PROT_WRITE,
				      MAP_SHARED | (startaddr != NULL ? MAP_FIXED : 0),
				      _backingFd,
				      0);

#ifndef NDEBUG
    //    printf ("transient = %p, persistent = %p, size = %ld\n", _transientMemory, _persistentMemory, NElts * sizeof(Type));
#endif

    // Finally, map the version numbers.
    _persistentVersions = (int *)
      mmap (NULL,
	    VersionArrayLength * sizeof(int),
	    PROT_READ | PROT_WRITE,
	    MAP_SHARED,
	    _versionsFd,
	    0);

    _localVersions = (int *)
      mmap (NULL,
	    VersionArrayLength * sizeof(int),
	    PROT_READ | PROT_WRITE,
	    MAP_PRIVATE,
	    _versionsFd,
	    0);


    if ((_transientMemory == MAP_FAILED) ||
	(_persistentMemory == MAP_FAILED) ||
	(_localVersions == MAP_FAILED) ||
	(_persistentVersions == MAP_FAILED)) {
      // If we couldn't map it, something has seriously gone wrong. Bail.
      ::abort();
    }

  }

  virtual ~xpersist (void) {
    // Unmap everything.
    munmap (_transientMemory,  NElts * sizeof(Type));
    munmap (_persistentMemory, NElts * sizeof(Type));
    munmap (_persistentVersions, VersionArrayLength * sizeof(int));
    close (_backingFd);
    close (_versionsFd);
  }

  // Go...
  void initialize (void) {
#if 0
    if (!_initialized) {
      // Set the mapping of transient memory to private before we really start.
      _transientMemory = (Type *) mmap (_transientMemory,
					NElts * sizeof(Type),
					PROT_READ,
					MAP_PRIVATE | MAP_FIXED,
					_backingFd,
					0);
      _initialized = true;
    }
#endif
  }

  /// @return true iff the address is in this space.
  inline bool inRange (void * addr) {
    if (((size_t) addr >= (size_t) base())
	&& ((size_t) addr < (size_t) base() + size())) {
      return true;
    } else {
      return false;
    }
  }


  /// @return the start of the memory region being managed.
  inline Type * base (void) const {
    return _transientMemory;
  }


  /// @return the size in bytes of the underlying object.
  inline int size (void) const {
    return NElts * sizeof(Type);
  }


  /// @brief Record a read to this location.
  void recordRead (void * addr) {
    if (inRange (addr)) {
      int index = (size_t) addr - (size_t) base();
      // Compute the page address of this item,
      // and mark the page as having been read.
      int pageNo = computePage (index);
      // Force a read of the version number.
      _localVersions[pageNo] = _persistentVersions[pageNo];
      _read.insert (pageNo);
    }
  }

  /// @brief Record a write to this location.
  void recordWrite (void * addr) {
    if (inRange (addr)) {
      int index = (size_t) addr - (size_t) base();
      // Compute the page address of this item,
      // and mark the page as being dirtied (so we commit it later).
      int pageNo = computePage (index);
      _dirtied.insert (pageNo);
      // Just to be on the safe side, we insert the page into the read set as well.
      _read.insert (pageNo);
    }
  }


  /// @brief Start a transaction.
  void begin (void) {
    // Clear the read and write (dirtied) page sets.
    _read.clear();
    _dirtied.clear();
  }
  

  /// @brief Fail a transaction (reverting local changes).
  /// @note This should happen only after calling consistent().
  void abort (void) {
    // Revert the local copies to the shared versions.
    updateAll();
  }

  bool nop (void) {
    bool noReads = _read.empty();
    bool noWrites = _dirtied.empty();
#if 0
    if (!noReads) {
      printf ("we read something.\n");
      
      for (pageSetType::iterator i = _read.begin();
	   i != _read.end();
	   ++i) {
	int pageNo = *i;
	printf ("[%d] read page %p\n", getpid(), (pageNo * 4096 + base()));
      }
    }
#endif
#if 0
    if (!noWrites) {
      printf ("we dirtied something.\n");
    }
#endif
    return (noReads && noWrites);
  }

  void stats (void) {
#if 0
    char buf[255];
    sprintf (buf, "xpersist stats: %d reads, %d dirtied\n", _read.size(), _dirtied.size());
    fprintf (stderr, buf);
#endif
  }

  /// @return true iff our version of the world is consistent.
  /// @note Requires that the lock be held.
  bool consistent (void) {
    assert (isLocked());

    memoryBarrier();

    // Check to see if every version of any page that's been read is
    // the most current version number.

    bool wasConsistent = true;

    pageSetType::iterator i;

    for (i = _read.begin();
	 i != _read.end();
	 ++i) {
      int pageNo = *i;

      if (_persistentVersions[pageNo] != _localVersions[pageNo]) {

	// Our view is not consistent.

#if 0 // !defined(NDEBUG)
	printf ("inconsistent page = %d (addr = [%p])\n", pageNo, (void *) ((pageNo * 4096) + base()));
	printf ("my version = %d, committed version = %d\n",
		_localVersions[pageNo],
		_persistentVersions[pageNo]);
#endif

	bool different = true; // (memcmp ((char *) _persistentMemory + xdefines::PageSize * pageNo, (char *) _transientMemory + xdefines::PageSize * pageNo, xdefines::PageSize) != 0);

	if (different) {

#if 0
	  printf ("diffs:\n");
	  for (int i = 0; i < xdefines::PageSize; i++) {
	    if (_transientMemory[pageNo * xdefines::PageSize + i] !=
		_persistentMemory[pageNo * xdefines::PageSize + i]) {
	      printf ("committed = %d, mine = %d\n",
		      _persistentMemory[pageNo * xdefines::PageSize + i],
		      _transientMemory[pageNo * xdefines::PageSize + i]);
	      
	    }
	  }
#endif

	  wasConsistent = false;
	  break;
	} else {
	  // No diffs - update local version number.
	  _localVersions[pageNo] = _persistentVersions[pageNo];
	}
      }
    }

    memoryBarrier();

    return wasConsistent;
  }


  /// @brief Force a commit of any modifications to the persistent store.
  /// @note Requires that the lock be held.
  void commit (void) {
    assert (isLocked());

    memoryBarrier();

    // Commit any local modifications.
    for (pageSetType::iterator i = _dirtied.begin();
	 i != _dirtied.end();
	 ++i) {

      // Write the page into persistent memory.
      int pageNo = *i;

      memcpy ((char *) _persistentMemory + xdefines::PageSize * pageNo,
	      (char *) _transientMemory + xdefines::PageSize * pageNo,
	      xdefines::PageSize);
      
      // This is now a new version, so increment our local version
      // number and record that to the persistent store.
      
      assert (pageNo >= 0);
      assert (pageNo < VersionArrayLength);
      
      _persistentVersions[pageNo] = _localVersions[pageNo] + 1;
      
    }

    // Write out all changes to the persistent memory and versions.

#if USE_MSYNC
    msync (_persistentMemory, NElts * sizeof(Type), MS_SYNC);
    msync (_persistentVersions, NElts * sizeof(int), MS_SYNC);
#endif

    // Dump the now-unnecessary page frames, reducing space overhead.
    pageSetType::iterator i;
    for (i = _dirtied.begin(); i != _dirtied.end(); ++i) {
      updatePage (*i);
    }

    memoryBarrier();
  }


  /// @brief Lock the store.
  void lock (void) {
    //    pthread_mutex_lock (&_theLock);
    // NOTE: we could refine the locks to cover smaller regions
    // of the various files. We could also do something more insane:
    // use real locking (pessimistic concurrency) and lock everything
    // up to each page we touch, whenever we read or write a page.
    // At the end of each transaction, unlock all locks.
    // Can randomization play a useful role here?
    // How this would work: we could read lock the whole region.
    // Then, we could write lock selectively (page n => write lock 0-n).
    // An alternative would be to do speculative execution...
    _lock.lock();
    _isLocked = true;
  }

  /// @brief Unlock the store.
  void unlock (void) {
    _lock.unlock();
    _isLocked = false;
  }

  bool isLocked (void) const {
    return _isLocked;
  }

  void commitMemory (void) {
    memcpy ((char *) _persistentMemory,
	    (char *) _transientMemory,
	    _startsize);
#if USE_MSYNC
    msync (_persistentMemory, NElts * sizeof(Type), MS_SYNC);
#endif
  }

  /// @brief Update every page frame from the backing file.
  void updateAll (void) {
    // Unmap and remap the transient memory as protected.
    madvise ((caddr_t) _localVersions, VersionArrayLength, MADV_DONTNEED);
    munmap (_transientMemory, NElts * sizeof(Type));
    mmap (_transientMemory,
	  NElts * sizeof(Type),
	  PROT_NONE, // PROT_READ | PROT_WRITE | PROT_EXEC,
	  MAP_PRIVATE | MAP_FIXED,
	  _backingFd,
	  0);
  }


  /// @brief Commit all writes.
  inline void memoryBarrier (void) {
    xatomic::memoryBarrier();
  }

private:

  inline int computePage (int index) {
    return (index * sizeof(Type)) / xdefines::PageSize;
  }


  /// @brief Update the given page frame from the backing file.
  void updatePage (int pageNo) {
    madvise (_transientMemory + pageNo * xdefines::PageSize, xdefines::PageSize, MADV_DONTNEED);
    mprotect (_transientMemory + pageNo * xdefines::PageSize, xdefines::PageSize, PROT_NONE);

#if 0
    printf ("updating: %d - local = %d, persistent = %d\n",
	    pageNo, _localVersions[pageNo], _persistentVersions[pageNo]);
#endif
  }

  /// The length of the version array, which has one entry per page.
  enum { VersionArrayLength = (NElts * sizeof(Type) + xdefines::PageSize-1) / xdefines::PageSize };

  /// The type of read and dirtied sets.
  typedef std::set<int, less<int>, HL::STLAllocator<int, privateheap> >
  pageSetType;

  /// True iff the lock is currently held.
  bool _isLocked;

  /// The starting address of the region.
  void * const _startaddr;

  /// The size of the region.
  const size_t _startsize;

  /// A map of read pages.
  pageSetType _read;

  /// A map of dirtied pages.
  pageSetType _dirtied;

  /// The file descriptor for the backing store.
  int _backingFd;

  /// The file descriptor for the versions.
  int _versionsFd;

  /// A lock that protects the file.
  xplock _lock;

  /// The transient (not yet backed) memory.
  Type * _transientMemory;

  /// The persistent (backed to disk) memory.
  Type * _persistentMemory;

  /// Local version numbers for each page.
  int * _localVersions; // [VersionArrayLength];

  /// The version numbers that are backed to disk.
  int * _persistentVersions;

  bool _initialized;

};


#endif
