#ifndef _ROUNDUP_H_
#define _ROUNDUP_H_

/**
 * @class roundup
 * @brief Increases the size (in bytes) of a class to a multiple of Size.
 *
 * @author Emery Berger <http://www.cs.umass.edu/~emery>
 * @note   Useful when we need to cache or page-align a class.
 */

template <int Size, class Parent>
class roundup : public Parent {
private:
  enum { SIZE = (Size * ((sizeof(Parent) + Size - 1) / Size))
	 - sizeof(Parent) };
  char _dummy[SIZE];
};

#endif
