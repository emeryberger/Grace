#ifndef _XSEMAPHORE_H_
#define _XSEMAPHORE_H_

#include <sys/sem.h>

class xsemaphore {
public:

  xsemaphore (int key = SEMAPHORE_KEY)
    : _key (key)
  {
  }

  void init (int num) 
  {
    // Initialize semaphore to the desired number.
    _arg.val = num;
    int semid = semget (_key, 1, IPC_CREAT | IPC_PRIVATE | 0666);
    semctl (semid, 0, SETVAL, _arg);
  }

  void get (void) {
    incSemaphore (-1);
  }

  void put (void) {
    incSemaphore (1);
  }

private:

  void incSemaphore (int val) {
    // Get the semaphore.
    int semid = semget (_key, 1, 0666);

    // Now increment the semaphore by the desired value.
    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_op = val;
    sops.sem_flg = 0;
    int retval = semop (semid, &sops, 1);
  }

  enum { SEMAPHORE_KEY = 1234567 };

  int _key;
  union semun {
    int val;
    struct semid_ds *buf;
    ushort * array;
  } _arg;

};

#endif
