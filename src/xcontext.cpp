#include "xcontext.h"
#include <stdlib.h>

NO_INLINE void xcontext::commit (void) {
  getContext();
}

NO_INLINE void xcontext::abort (void) {
  restoreStack(1);
}

void * xcontext::stacktop (void)
{
#if defined(sparc) && defined(sun)
  register unsigned long sp;
  __asm__ __volatile__("mov %%sp, %0" : "=r" (sp));
  return (void *) sp;
#elif defined(__i386__)
  register unsigned long sp;
  __asm__ __volatile__("movl %%esp, %0" : "=r" (sp));
  return (void *) sp;
#elif defined(__x86_64__)
  register unsigned long sp;
  __asm__ __volatile__("movq %%rsp, %0" : "=r" (sp)); 
  return (void *) sp;
#else
  /* Adapted from http://www.awprofessional.com/articles/article.asp?p=606582&seqNum=4&rl=1 */
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
#endif
}

NO_INLINE void xcontext::initialize (void) {
  _pbos = (unsigned long *) stacktop();
  commit();
}

NO_INLINE void xcontext::save_stack (unsigned long *pbos, unsigned long *ptos) {
  int n = pbos-ptos;
  int i;
  _stackSize = n;
  
  for (i = 0; i < n; ++i) 
    {
      _stack[i] = pbos[-i];
    }
}

NO_INLINE void xcontext::getContext (void) {
  volatile unsigned long tos;
  // First, save registers (context).
  if (!getcontext(&_registers)) {
    // Now, save the stack.
    save_stack (_pbos, (unsigned long *) &tos);
  }
}

NO_INLINE void xcontext::restoreStack (int once_more) {
  long padding[12];
  unsigned long tos;
  
  if (_pbos-_stackSize < &tos) {
    restoreStack (1);
  }
  
  if (once_more) {
    restoreStack (0);
  }
  
  // Copy stack back out from continuation.
  for (int i = 0; i < _stackSize; ++i) {
    _pbos[-i] = _stack[i];
  }
  setcontext (&_registers);
}
