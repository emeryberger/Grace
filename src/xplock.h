#ifndef _XPLOCK_H_
#define _XPLOCK_H_

#if !defined(_WIN32)
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @class xplock
 * @brief A cross-process lock.
 *
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 */

// Use secret hidden APIs to avoid mallocs...

extern "C" {

#if defined(sparc)

#define WRAP(x) _##x
  void _pthread_mutex_init (void *, void *);
  void _pthread_mutex_lock (void *);
  void _pthread_mutex_unlock (void *);

#elif defined(__APPLE__)

  //  #define WRAP(x) _new_##x
  //  void _new_pthread_mutex_init (void *, void *);
  //  void _new_pthread_mutex_lock (void *);
  //  void _new_pthread_mutex_unlock (void *);
#define WRAP(x) _##x
  void _pthread_mutex_init (void *, void *);
  void _pthread_mutex_lock (void *);
  void _pthread_mutex_unlock (void *);

#else

#define WRAP(x) __##x
  //  void __pthread_mutexattr_init (void *);
  void __pthread_mutex_init (void *, void *);
  void __pthread_mutex_lock (void *);
  void __pthread_mutex_unlock (void *);
#endif
}

class xplock {
private:

  // We can use pthread-based locks, or filesystem-based locks.
  enum { USE_PTHREAD_LOCKS = true };

public:

  xplock (void) {
    char fname[255];

    // Create an appropriately-sized file.
    sprintf (fname, "/tmp/xplokXXXXXX");
    _backingFd = mkstemp (fname);
    int result = ftruncate (_backingFd, 4096);
    unlink (fname);

    if (result) {
      fprintf (stderr, "Couldn't create lock file.\n");
      ::abort();
    }

    // Instantiate the lock structure inside a shared mmap.
    // Note: we assume that the lock is no more than 4K in size.
    _lock = (pthread_mutex_t *) mmap (NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, _backingFd, 0);

    // Set up the lock with a shared attribute.
    // NOTE: Init below was 'wrapped' for POSIX...
    pthread_mutexattr_init(&_attr);
    pthread_mutexattr_setpshared (&_attr, PTHREAD_PROCESS_SHARED);
    WRAP(pthread_mutex_init) (_lock, &_attr);

    if (!USE_PTHREAD_LOCKS) {
      // Now initialize the lock.
      _fileLock.l_whence = SEEK_SET;
      // Lock from start of file...
      _fileLock.l_start = 0;
      // to the end.
      _fileLock.l_len = 0; 
    }
  }

  /// @brief Lock the lock.
  void lock (void) {
    if (USE_PTHREAD_LOCKS) {
      WRAP(pthread_mutex_lock) (_lock);
    } else {
      _fileLock.l_type = F_WRLCK;
      int r = fcntl (_backingFd, F_SETLK, &_fileLock);
      if (r == -1) {
	r = fcntl (_backingFd, F_SETLKW, &_fileLock);
      }
    }
    _isLocked = true;
  }

  /// @brief Unlock the lock.
  void unlock (void) {
    if (USE_PTHREAD_LOCKS) {
      WRAP(pthread_mutex_unlock)(_lock);
    } else {
      _fileLock.l_type = F_UNLCK;
      fcntl (_backingFd, F_SETLKW, &_fileLock);
    }
    _isLocked = false;
  }

private:

  /// True iff the lock is currently held.
  bool _isLocked;
  
  /// A lock that protects the file.
  struct flock _fileLock;

  /// The file descriptor for the lock.
  int _backingFd;

  /// A pointer to the lock.
  pthread_mutex_t * _lock;

  /// The lock's attributes.
  pthread_mutexattr_t _attr;

};


#endif
