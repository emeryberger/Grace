// -*- C++ -*-

#ifndef _XSECTION_H_
#define _XSECTION_H_

class xsection {
public:

  xsection (void)
  {
  }

  unsigned int offset;
  unsigned int length;
  void * buf;
};


#endif /* _XSECTION_H_ */
