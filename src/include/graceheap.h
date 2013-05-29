// -*- C++ -*-

#include "xdefines.h"

#include "xpersist.h"
#include "xoneheap.h"
#include "bumpheap.h"
#include "warpheap.h"
#include "mmapheap.h"
#include "roundup.h"

#include "ansiwrapper.h"

class graceheapHelper {

private:

  // Allow only one malloc, and initialize memory (via commitMemory)
  // lazily (only once malloc is called).
  template <class S>
  class AdaptToXpersist : public S {
    bool _alreadyMalloced;
  public:
    AdaptToXpersist (void)
      : _alreadyMalloced (false)
    {}
    void * malloc (size_t) {
      if (_alreadyMalloced) {
	return NULL;
      } else {
	_alreadyMalloced = true;
	S::commitMemory();
	return S::base();
      }
    }
  private:
    void free (void);
  };

  // We work our way from bottom up.
  // First, we define the persistent heap (HEAP_SIZE bytes)
  // that actually stores the data.
  class persistentHeap :
    public xpersist<char, xdefines::HEAP_SIZE> {};

  // Next, we adapt this heap, using the adapter class above,
  // which makes sure the persistent heap gets initialized
  // lazily.
  class adaptedHeap :
    public AdaptToXpersist<persistentHeap> {};

  // Next, we define a bump-pointer source so we can use this like
  // an ordinary region-style heap.
  class bumpyHeap :
    public bumpheap<xdefines::HEAP_SIZE,
		    adaptedHeap> {};


  // Now we round-up the size of that data structure to a multiple
  // of a page, so that the heap metadata occupies separate pages.

public:

  class roundedHeap :
    public roundup<xdefines::PageSize, bumpyHeap> {};

};

class graceheap :
  public HL::ANSIWrapper<
  warpheap<xdefines::NUM_HEAPS, xoneheap<graceheapHelper::roundedHeap> > > {};

// An alternative:
// xoneheap<bumpyHeap>  _heap;

