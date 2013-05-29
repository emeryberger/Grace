// -*- C++ -*-

#ifndef _XTHREAD_H_
#define _XTHREAD_H_

#include <errno.h>

#if !defined(_WIN32)
#include <sys/wait.h>
#include <sys/types.h>
#endif

#include <stdlib.h>

#include "xdefines.h"
#include "xlatch.h"
#include "xsemaphore.h"

// Misc
#include "log.h"

// Heap Layers

#include "freelistheap.h"
#include "zoneheap.h"
#include "mmapheap.h"
#include "util/cpuinfo.h"

extern "C" {
  // The type of a pthread function.
  typedef void * threadFunction (void *);
}

class xrun;


class xthread {
private:

  /// @class ThreadStatus
  /// @brief Holds the thread id and the return value.
  class ThreadStatus {
  public:
    ThreadStatus (void * r, bool f)
      : retval (r),
	forked (f)
    {}

    ThreadStatus (void)
    {}

    /// The thread id.
    int tid;

    /// The return value from the thread.
    void * retval;

    /// Whether this thread was created by a fork or not.
    bool forked;
  };

public:

  xthread (void)
    : _nestingLevel (0)
  {
#if USE_XLATCH
    initExited();
#endif
  }

  void setMaxThreads (unsigned int n)
  {
    //    _throttle.set (n);
    //    printf ("throttle = %d\n", n);
  }

  void * spawn (xrun * runner,
		threadFunction * fn,
		void * arg);

  void sync (xrun * runner,
	     void * v,
	     void ** result);

  inline int getId (void) const {
    return _tid;
  }

  inline void setId (int id) {
    _tid = id;
  }

#if USE_XLATCH

  /// An array to keep track of whether a thread has exited yet.
  xlatch * _threadExitStatus;

  /// How many elements in the thread-exited array?
  enum { THREAD_STATUS_LENGTH = 1048576 };

  /// @brief Initialize the data structure to keep track of thread exits.
  void initExited (void) {
    void * buf = allocateSharedObject (sizeof(xlatch) * THREAD_STATUS_LENGTH);
    _threadExitStatus = new (buf) xlatch[THREAD_STATUS_LENGTH];
    for (int i = 0; i < THREAD_STATUS_LENGTH; i++) {
      new ((void *) &_threadExitStatus[i]) xlatch;
    }
  }

  /// @brief Wait until a given process has exited.
  inline void waitExited (int pid) {
    _threadExitStatus[pid % THREAD_STATUS_LENGTH].wait();
  }

  /// @brief Indicate that this process has exited.
  inline void setExited (void) {
    _threadExitStatus[getId() % THREAD_STATUS_LENGTH].unlatch();
  }

#else

  inline void setExited (void) {
  }

  /// @brief Wait until a given process has exited.
  static void waitExited (int pid) {

    // First, try to wait for the pid directly. This approach only
    // works if the process is our child.

    int r = waitpid (pid, NULL, 0);
    if (r == pid) {
      return;
    }

    // The process wasn't our child, so we use an alternate method.
    // We repeatedly test to see if we can send a signal to the process,
    // via kill(0). If so, we sleep for a stretch.

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000;
    while (true) {
      int r = kill (pid, 0);
      if ((r == -1) && (errno == ESRCH)) {
	// No such process -- i.e., it's done. Return.
	return;
      }
      nanosleep (&ts, NULL);
    }
  }
#endif

private:

  void * forkSpawn (xrun * runner,
		    threadFunction * fn,
		    ThreadStatus * t,
		    void * arg);

  static void run_thread (xrun * runner,
			  threadFunction * fn,
			  ThreadStatus * t,
			  void * arg);

  inline bool dontFork (void) const {
    static unsigned int lognCPUs = log(HL::CPUInfo::getNumProcessors());
    return (_nestingLevel >= lognCPUs);
  }

  // A semaphore that bounds the number of active threads at any time.
  //  xsemaphore	   _throttle;

  /// A heap that just holds pages to hold thread results.
  HL::FreelistHeap<HL::ZoneHeap<HL::MmapHeap, 4096> > _tstatHeap;

  /// Current nesting level (i.e., how deep we are in recursive threads).
  unsigned int	   _nestingLevel;

  /// What is this thread's PID?
  int              _tid;

  /// @return a chunk of memory shared across processes.
  void * allocateSharedObject (size_t sz) {
    return mmap (NULL,
		 sz,
		 PROT_READ | PROT_WRITE,
		 MAP_SHARED | MAP_ANONYMOUS,
		 -1,
		 0);
  }

  void freeSharedObject (void * ptr, size_t sz) {
    munmap (ptr, sz);
  }

};

#endif
