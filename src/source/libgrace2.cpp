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

#if !defined(_WIN32)
#include <dlfcn.h>
#endif

#include <stdarg.h>
#include "xrun.h"

#ifndef DETERMINISTIC_THREADS
#define DETERMINISTIC_THREADS 0 // FIX ME!!
#endif

static bool isInitialized = false;

#define REPLACE_HEAP_FUNCTIONS 1
#define REPLACE_IO_FUNCTIONS 0

xrun * theRunner = NULL;
static char theRunnerBuf[sizeof(xrun)];


extern "C" {

#if defined(__GNUG__)
  void initializer (void) __attribute__((constructor));
  void finalizer (void)   __attribute__((destructor));
#endif

  void graceinitialize (volatile void * d) {
    if (!isInitialized) {
      theRunner = new (theRunnerBuf) xrun;
      theRunner->initialize (d);
      isInitialized = true;
    }
  }

  void gracebegin (void) {
#if 0
    if (!isInitialized) {
      initializer();
    }
#endif
    theRunner->atomicBegin ();
  }

  void graceresume (void) {
#if 0
    if (!isInitialized) {
      initializer();
    }
#endif
    gracebegin();
  }

 
  /* From http://www.awprofessional.com/articles/article.asp?p=606582&seqNum=4&rl=1 */
  static 
  __attribute__ ((noinline)) void * stacktop (void)
  {
    unsigned int level = 1;
    void    *saved_ra  = __builtin_return_address(0);
    void   **fp;
    void    *saved_fp  = __builtin_frame_address(0);
    
    fp = (void **) saved_fp;
    while (fp) {
      saved_fp = *fp;
      fp = (void **) saved_fp;
      if ((fp == NULL) ||
	  (*fp == NULL))
	break;
      saved_ra = *(fp + 2);
      level++;
    }
    return saved_fp;
  }

  void initializer (void) {
    // Grab the top of the stack.
    volatile void * p = stacktop();
    // Initialize everything.
    graceinitialize (p);
    // Start our first transaction.
#ifndef NDEBUG
    printf ("we're gonna begin now.\n"); fflush (stdout);
#endif
    gracebegin();
  }

  void * gracemalloc (size_t sz) {
#if 0
    if (!isInitialized) {
      initializer();
    }
#endif
    return theRunner->malloc (sz);
  }

  void gracefree (void * ptr) {
#if 0
    if (!isInitialized) {
      initializer();
    }
#endif
    theRunner->free (ptr);
  }

  size_t gracemalloc_usable_size (void * ptr) {
#if 0
    if (!isInitialized) {
      initializer();
    }
#endif
    return theRunner->getSize (ptr);
  }

  void gracecondinit (void ** c)
  {
    bool * p = (bool *) mmap (NULL,
		 4096,
		 PROT_READ | PROT_WRITE,
		 MAP_SHARED | MAP_ANONYMOUS,
		 -1,
		 0);
    p[0] = false;
    *c = (void *) p;
  }

  void graceconddestroy (void * c)
  {
    void * ptr = (void *) *((void **) c);
    munmap (ptr, 4096);
  }

  void gracesignal (void * c)
  {
#if 0
    if (!isInitialized) {
      initializer();
    }
#endif
    theRunner->punctuate();
    bool * p = (bool *) *((void **) c);
    p[0] = true;
    theRunner->punctuate();
  }

  void gracewait (void * c)
  {
#if 0
    if (!isInitialized) {
      initializer();
    }
#endif
    bool * p = (bool *) *((void **) c);
    do {
      theRunner->punctuate();
      if (p[0]) {
	break;
      }
      sleep (1);
    } while (true);
    p[0] = false;
    theRunner->punctuate();
  }


  void * gracespawn (void *(*fn) (void *), void * arg)
  {
#if 0
    if (!isInitialized) {
      initializer();
    }
#endif
    return theRunner->spawn (fn, arg);
  }

  void gracesync (void * v, void ** val) {
#if 0
    if (!isInitialized) {
      initializer();
    }
#endif
    return theRunner->sync (v, val);
  }

  void finalizer (void) {
#if 0
    if (!isInitialized) {
      initializer();
    }
#endif
    theRunner->finalize();
  }

  int graceid (void) {
#if 0
    if (!isInitialized) {
      initializer();
    }
#endif
    return theRunner->id();
  }

  
#if !defined(__SVR4)
#define NOTHROW throw()
#else
#define NOTHROW
#endif

#if 1

  int sched_yield (void) NOTHROW
  {
    return 0;
  }

  void pthread_exit (void * value_ptr) {
    // FIX ME?
    // This should probably throw a special exception to be caught in spawn.
  }

  int pthread_setconcurrency (int) {
    return 0;
  }

  int pthread_attr_init (pthread_attr_t *) {
    return 0;
  }

  int pthread_attr_destroy (pthread_attr_t *) {
    return 0;
  }

  pthread_t pthread_self (void) NOTHROW
  {
    return (pthread_t) graceid();
  }

  int pthread_kill (pthread_t thread, int sig) {
    // FIX ME
    throw 1;
  }

#if 0
  int pthread_equal (pthread_t t1, pthread_t t2) NOTHROW {
    return (t1 == t2);
  }
#endif

  int pthread_mutex_init (pthread_mutex_t *,
			  const pthread_mutexattr_t *)
  {
    return 0;
  }

  int pthread_mutex_lock (pthread_mutex_t *) NOTHROW
  {
    return 0;
  }
  
  int pthread_mutex_trylock (pthread_mutex_t *) NOTHROW
  {
    return 0;
  }

  int pthread_mutex_unlock (pthread_mutex_t *) NOTHROW
  {
    return 0;
  }

  int pthread_rwlock_destroy (pthread_rwlock_t * rwlock) NOTHROW
  {
    return 0;
  }

  int pthread_rwlock_init (pthread_rwlock_t * rwlock,
			   const pthread_rwlockattr_t * attr) NOTHROW
  {
    return 0;
  }

  int pthread_detach (pthread_t thread) NOTHROW
  {
    return 0;
  }

  int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock) NOTHROW
  {
    return 0;
  }

  int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock) NOTHROW
  {
    return 0;
  }


  int pthread_rwlock_unlock(pthread_rwlock_t *rwlock) NOTHROW
  {
    return 0;
  }


  int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock) NOTHROW
  {
    return 0;
  }

  int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock) NOTHROW
  {
    return 0;
  }

  int pthread_attr_getstacksize (const pthread_attr_t *, size_t * s) {
    *s = 1048576UL; // really? FIX ME
    return 0;
  }

  int pthread_mutexattr_destroy (pthread_mutexattr_t *) { return 0; }
  int pthread_mutexattr_init (pthread_mutexattr_t *)    { return 0; }
  int pthread_mutexattr_settype (pthread_mutexattr_t *, int) { return 0; }
  int pthread_mutexattr_gettype (const pthread_mutexattr_t *, int *) { return 0; }
  int pthread_attr_setstacksize (pthread_attr_t *, size_t) { return 0; }

  int pthread_create (pthread_t * tid,
		      const pthread_attr_t * attr,
		      void *(*start_routine) (void *),
		      void * arg) NOTHROW
  {
    *tid = (pthread_t) gracespawn (start_routine, arg);
    return 0;
  }

  int pthread_join (pthread_t tid, void ** val) {
    gracesync ((void *) tid, val);
    return 0;
  }

#if 0
  int pthread_cond_init (pthread_cond_t * cond, const pthread_condattr_t *) {
#if DETERMINISTIC_THREADS
    bool * v = (bool *) cond;
    *v = false;
    cond = (pthread_cond_t *) v;
#endif
    return 0;
  }

  int pthread_cond_broadcast (pthread_cond_t * cond) {
#if DETERMINISTIC_THREADS
    // Make any updates visible to other threads.
    bool * v = (bool *) cond;
    *v = true;
    theRunner->punctuate();
#endif
    return 0; // gracebroadcast (cond);
  }

  int pthread_cond_signal (pthread_cond_t * cond) {
#if DETERMINISTIC_THREADS
    bool * v = (bool *) cond;
    *v = true;
    // Make any updates visible to other threads.
    theRunner->punctuate();
#endif
    return 0; //gracesignal (cond);
  }

  int pthread_cond_wait (pthread_cond_t * cond,
			 pthread_mutex_t * mutex) {
#if DETERMINISTIC_THREADS
    if (!isInitialized) 
      return 0;
    do {
      // Make any updates visible to other threads.
      theRunner->punctuate();
      if (*((bool *) cond)) {
	break;
      }
      sleep (1);
    } while (true);
#endif

    return 0; // gracewait (cond);
  }

  int pthread_cond_destroy (pthread_cond_t * cond) {
    return 0; // graceconddestroy (cond);
  }
#endif

}

#if REPLACE_HEAP_FUNCTIONS

extern "C" {
  void * malloc (size_t sz) {
#if 1
    if (!isInitialized) {
      return NULL;
    }
#endif
    if (sz < sizeof(double)) {
      sz = sizeof(double);
    }
    return gracemalloc (sz);
  }

  void free (void * ptr) {
    gracefree (ptr);
  }

  void * calloc (size_t n, size_t s) {
    void * ptr = gracemalloc (n * s);
    if (ptr) {
      memset (ptr, 0, n * s);
    }
    return ptr;
  }

  void * realloc (void * ptr, size_t sz) {
    if (ptr == NULL) {
      return gracemalloc (sz);
    }
    if (sz == 0) {
      gracefree (ptr);
      return NULL;
    }
    size_t s = gracemalloc_usable_size (ptr);
    void * newptr = gracemalloc (sz);
    if (newptr) {
      size_t copySz = (s < sz) ? s : sz;
      memcpy (newptr, ptr, copySz);
    }
    gracefree (ptr);
    return newptr;
  }

  ////// I/O operations

  int graceopen (char *pathname, int flags) {
    return theRunner->open (pathname, flags);
  }
  
  int graceclose (int fd) {
    return theRunner->close (fd);
  }

  int graceread (int fd, void * buf, int count) {
    return theRunner->read (fd, buf, count);
  }

  int gracewrite (int fd, void * buf, int count) {
    return theRunner->write (fd, buf, count);
  }
  
  int gracelseek (int fd, int offset, int whence) {
    return theRunner->lseek (fd, offset, whence);
  }
  
  int graceconnect (int fd, struct sockaddr *serv_addr, socklen_t addrlen) {
    return theRunner->connect (fd, serv_addr, addrlen);
  }
  
  int graceaccept (int fd, struct sockaddr *addr, socklen_t * addrlen) {
    return theRunner->accept (fd, addr, addrlen);
  }

}


void * operator new (size_t sz)
{
 void * ptr = gracemalloc (sz);
  if (ptr == NULL) {
    throw std::bad_alloc();
  } else {
    return ptr;
  }
}

void operator delete (void * ptr)
{
  gracefree (ptr);
}

#if !defined(__SUNPRO_CC) || __SUNPRO_CC > 0x420
void * operator new (size_t sz, const std::nothrow_t&) throw() {
  return gracemalloc (sz);
} 

void * operator new[] (size_t size) throw(std::bad_alloc)
{
  void * ptr = gracemalloc (size);
  if (ptr == NULL) {
    throw std::bad_alloc();
  } else {
    return ptr;
  }
}

void * operator new[] (size_t sz, const std::nothrow_t&) throw() {
  return gracemalloc (sz);
} 

void operator delete[] (void * ptr)
{
  gracefree (ptr);
}
#endif


#endif


#if REPLACE_IO_FUNCTIONS

extern "C" typedef ssize_t (*writeFunction) (int, const void *, size_t);
extern "C" typedef ssize_t (*readFunction) (int, void *, size_t);
extern "C" typedef int (*openFunction1) (const char *, int, ...);
extern "C" typedef int (*openFunction2) (const char *, int);
extern "C" typedef int (*creatFunction) (const char *, mode_t);

extern "C" 
{

  static int local_open (const char * pathname, int flags, mode_t mode) {
    static openFunction1 realOpen = (openFunction1) dlsym (RTLD_NEXT, "open");
    int result = (realOpen)(pathname, flags, mode);
    return result;
  }

  static int local_creat (const char * pathname, mode_t mode) {
    static creatFunction realCreat = (creatFunction) dlsym (RTLD_NEXT, "creat");
    int result = (realCreat)(pathname, mode);
    return result;
  }

  static ssize_t local_write (int fd, const void * buf, size_t count) {
    static writeFunction realWrite = (writeFunction) dlsym (RTLD_NEXT, "write");
    ssize_t result = (realWrite)(fd, buf, count);
    return result;
  }

  static ssize_t local_read (int fd, void * buf, size_t count) {
    static readFunction realRead = (readFunction) dlsym (RTLD_NEXT, "read");
    ssize_t result = (realRead)(fd, buf, count);
    return result;
  }

  int local_puts (const char * str) {
    char buf[2048]; // FIX ME THIS IS OBVIOUSLY NO GOOD.
    sprintf (buf, "%s\n", str);
    local_write (1, buf, strlen(buf));
    return 0;
  }

  int creat (const char * pathname, mode_t mode) {
    gracepause();
    int result = local_creat (pathname, mode);
    graceresume();
    return result;
  }

  int puts (const char * s) {
    char buf[2048];
    int len = strlen(s);
    memcpy (buf, s, len);
    buf[len] = '\n';
    gracepause();
    local_write (1, (void *) buf, len+1);
    graceresume();
    return 0;
  }


  int printf (const char * format, ...) {
    gracepause();
    va_list ap;
    va_start (ap, format);
    int v = vprintf (format, ap);
    va_end (ap);
    graceresume();
    return v;
  }

  ssize_t write (int fd, const void * buf, size_t count) {
    gracepause();
    local_write (fd, buf, count);
    graceresume();
    return count;
  }
  
  ssize_t read (int fd, void * buf, size_t count) {
    gracepause();
    ssize_t sz = local_read (fd, buf, count);
    graceresume();
    return sz;
  }
}

#endif

#endif
