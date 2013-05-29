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

#define REPLACE_HEAP_FUNCTIONS 1
#define REPLACE_IO_FUNCTIONS 1

extern "C" {

#if defined(__GNUG__)
  void initializer (void) __attribute__((constructor));
  void finalizer (void)   __attribute__((destructor));
#endif

  void initializer (void) {
    // Initialize everything.
    xrun::getInstance().initialize();
    // Start our first transaction.
#ifndef NDEBUG
    //    printf ("we're gonna begin now.\n"); fflush (stdout);
#endif
  }


  void finalizer (void) {
    xrun::getInstance().finalize();
  }


  void * gracemalloc (size_t sz) {
    return xrun::getInstance().malloc (sz);
  }

  void gracefree (void * ptr) {
    xrun::getInstance().free (ptr);
  }

  size_t gracemalloc_usable_size (void * ptr) {
    return xrun::getInstance().getSize (ptr);
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
    xrun::getInstance().punctuate();
    bool * p = (bool *) *((void **) c);
    p[0] = true;
    xrun::getInstance().punctuate();
  }

  void gracewait (void * c)
  {
    bool * p = (bool *) *((void **) c);
    do {
      xrun::getInstance().punctuate();
      if (p[0]) {
	break;
      }
      sleep (1);
    } while (true);
    p[0] = false;
    xrun::getInstance().punctuate();
  }


  void * gracespawn (void *(*fn) (void *), void * arg)
  {
    return xrun::getInstance().spawn (fn, arg);
  }

  void gracesync (void * v, void ** val) {
    return xrun::getInstance().sync (v, val);
  }

  int graceid (void) {
    return xrun::getInstance().id();
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
    _exit (0);
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

  void gracepunctuate (void) {
    xrun::getInstance().punctuate();
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
    *s = 1048576UL; // Arbitrary value for now.
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

}

#if REPLACE_HEAP_FUNCTIONS

extern "C" {
  void * malloc (size_t sz) {
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

#if 0
  int graceclose (int fd) {
    return xrun::getInstance().close (fd);
  }

  int graceread (int fd, void * buf, int count) {
    return xrun::getInstance().read (fd, buf, count);
  }

  int gracewrite (int fd, void * buf, int count) {
    return xrun::getInstance().write (fd, buf, count);
  }
  
  int gracelseek (int fd, int offset, int whence) {
    return xrun::getInstance().lseek (fd, offset, whence);
  }
  
  int graceconnect (int fd, struct sockaddr *serv_addr, socklen_t addrlen) {
    return xrun::getInstance().connect (fd, serv_addr, addrlen);
  }
  
  int graceaccept (int fd, struct sockaddr *addr, socklen_t * addrlen) {
    return xrun::getInstance().accept (fd, addr, addrlen);
  }
#endif

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
extern "C" 
{

#if 0
  static int open (const char * pathname, int flags, mode_t mode) {
    return xrun::getInstance().open (pathname, flags, mode);
  }

  static int creat (const char * pathname, mode_t mode) {
    return xrun::getInstance().creat (pathname, mode);
  }

  static ssize_t write (int fd, const void * buf, size_t count) {
    return xrun::getInstance().write (fd, buf, count);
  }

  static ssize_t read (int fd, void * buf, size_t count) {
    return xrun::getInstance().read (fd, buf, count);
  }

  int puts (const char * str) {
    return xrun::getInstance().puts (str);
  }
#endif

#if 1
  int printf (const char * format, ...) {
    xrun::getInstance().atomicEnd();
    va_list ap;
    va_start (ap, format);
    int v = vprintf (format, ap);
    va_end (ap);
    xrun::getInstance().atomicBegin();
    return v;
  }
#endif


}

#endif

#endif
