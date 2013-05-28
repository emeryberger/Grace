#ifndef _PPHEAP_H
#define _PPHEAP_H


template <int NumHeaps,
	  class TheHeapType>
class PPHeap : public TheHeapType {
public:

  PPHeap (void)
    : _initialized (false)
  {
  }

  void * malloc (size_t sz)
  {
    if (!_initialized) {
      for (int i = 0; i < NumHeaps; i++) {
	_heap[i].free (_heap[i].malloc (sizeof(double)));
      }
      _initialized = true;
    }
    // Try to get memory from the local heap first.
    int ind = thisHeap();
    //    printf ("malloc %d, total[%d] = %d\n", sz, ind, total[ind]);
    void * ptr = _heap[ind].malloc (sz);
    return ptr;
  }

  void free (void * ptr)
  {
    // Put the freed object onto this thread's heap.  Note that this
    // policy is essentially pure private heaps, (see Berger et
    // al. ASPLOS 2000), and so suffers from numerous known problems.
    int ind = thisHeap();
    _heap[ind].free (ptr);
  }

private:

  inline static int thisHeap (void) {
    int pid = (int) pthread_self();
    HL::sassert<((NumHeaps & (NumHeaps-1)) == 0)>
      NumHeaps_MUST_BE_POWER_OF_TWO;
    return (pid & (NumHeaps-1));
  }

  bool _initialized;
  TheHeapType _heap[NumHeaps];
};


#endif
