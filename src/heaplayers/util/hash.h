#ifndef _HL_HASH_H_
#define _HL_HASH_H_

#include <stdlib.h>

namespace HL {
  template <typename Key>
    extern size_t hash (Key k);
  
  template <>
    inline size_t hash (void * v) {
    return (size_t) v;
  }
  
  template <>
    inline size_t hash (size_t v) {
    return v;
  }
  
  template <>
    inline size_t hash (int v) {
    return (size_t) v;
  }
};

#endif
