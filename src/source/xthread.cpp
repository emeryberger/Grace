#include "xthread.h"
#include "xrun.h"

void * xthread::spawn (xrun * runner,
		       threadFunction * fn,
		       void * arg)
{
  // Decide whether we are going to use fork or just directly
  // execute the thread.
  
  // Adjust the number of processes we are allowed to spark.
  // Each increase in the nesting level makes us geometrically
  // less likely to fork off a new process.
  
  if (dontFork()) {
    
    // Enter the thread.
    _nestingLevel++;

    void * retval = fn (arg);

    // And leave the thread.
    _nestingLevel--;

    // Put the thread's return value in a specially-allocated
    // object.
    void * buf = (void *) _tstatHeap.malloc (sizeof(ThreadStatus));
    ThreadStatus * t = new (buf) ThreadStatus (retval, false);

    return t;

  } else {
    
    runner->atomicEnd();

    // Allocate an object to hold the thread's return value.
    void * buf = allocateSharedObject (4096);
    HL::sassert<(4096 > sizeof(ThreadStatus))> checkSize;
    ThreadStatus * t = new (buf) ThreadStatus;

    return forkSpawn (runner, fn, t, arg);

  }
}



/// @brief Wait for a thread.
void xthread::sync (xrun * runner,
		    void * v,
		    void ** result)
{

  // Return immediately if the thread argument is NULL.
  if (v == NULL) {
    return;
  }
   
  ThreadStatus * t = (ThreadStatus *) v;
    
  // We are done with this chunk of memory, so free it.
  bool didFork = t->forked;

  if (didFork) {

    // This was a real spawn:
    // wait for the 'thread' to terminate.
      
    runner->atomicEnd();
      
    if (t->tid) {
      waitExited (t->tid);
    }
  }

  // Grab the thread result from the status structure (set by the thread),
  // reclaim the memory, and return that result.
  if (result != NULL) {
    *result = t->retval;
  }
    
   
  if (didFork) {
    _tstatHeap.free (t);
    runner->atomicBegin();
  } else {
    freeSharedObject (t, 4096);
  }
}


void * xthread::forkSpawn (xrun * runner,
			   threadFunction * fn,
			   ThreadStatus * t,
			   void * arg) 
{
  t->forked = true;
  
  // Wait on the throttle semaphore.
  //  _throttle.get();

  // Use fork to create the effect of a thread spawn.
  int child = fork();
  
  if (child) {
    // I'm the parent (caller of spawn).
    
    // Store the tid so I can later sync on this thread.
    t->tid = child;
      
    // My logical predecessor is the child (i.e., I have to wait
    // for my child to commit before I can).
    runner->setPred (child);

    // Start a new atomic section and return the thread info.
    runner->atomicBegin();
    return (void *) t;
      
  } else {
    // I'm the spawned child who will run the thread function.

    // Set "thread_self".
    setId (getpid());

    // We're in...
    _nestingLevel++;

    // Run the thread...
    run_thread (runner, fn, t, arg);

    // and we're out.
    _nestingLevel--;

    // Wait for my logical predecessor, if any.
    runner->waitPred();
    
    // We're done here.
    setExited();

    // Allow another thread to run.
    //    _throttle.put();

    // And that's the end of this "thread".
    _exit (0);

    // Avoid complaints.
    return NULL;
  }
}

// @brief Execute the thread.
void xthread::run_thread (xrun * runner,
			  threadFunction * fn,
			  ThreadStatus * t,
			  void * arg) 
{
  // Run the thread inside a transaction.
  runner->atomicBegin();
  void * result = fn (arg);
  runner->atomicEnd();
    
  // We're done. Write the return value.
  t->retval = result;
}
