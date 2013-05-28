#ifndef _LOG_H_
#define _LOG_H_

static inline unsigned int log (unsigned int v) {
  unsigned int log = 0;
  unsigned int value = 1;
  while (value < v) {
    value <<= 1;
    log++;
  }
  return log;
}

#endif
