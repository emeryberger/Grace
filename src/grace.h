#include <stdlib.h>

#ifndef _GRACE_H_
#define _GRACE_H_

extern "C" {
  void gracebegin (void);
  void graceresume (void);
  void graceinitialize (volatile void *);
  void * gracemalloc (size_t);
  void gracefree (void *);
  size_t gracemalloc_usable_size (void *);
  void gracecondinit (void *);
  void gracesignal (void *);
  void gracewait (void *);
  void * gracespawn (void *(*fn) (void *), void * arg);
  void gracesync (void * v, void ** val);
  int graceid (void);

}

#endif
