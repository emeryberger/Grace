The Grace Safe Threading Library
================================
Copyright (c) 2007-13 Emery Berger, University of Massachusetts Amherst.

Emery Berger
<http://www.cs.umass.edu/~emery>

# Introduction #

Grace is a runtime system for safe and efficient multithreading.
Grace replaces the standard pthreads library with a new runtime
system that eliminates concurrency errors while maintaining good
scalability and high performance. Grace works with unaltered C/C++
programs, requires no compiler support, and runs on standard
hardware platforms. Grace can ensure the correctness of
otherwise-buggy multithreaded programs, and at the same time,
achieve high performance and scalability.

Grace *prevents* the following errors:

  * deadlocks
  * race conditions
  * atomicity violations
  * nondeterministic results

In effect, running with Grace makes a multithreaded program ACT like
each thread is executing sequentially, one after the other. However,
Grace can actually execute these threads concurrently, so that the
resulting program can scale up.

Grace currently works for multithreaded programs with FORK-JOIN
parallelism; in other words, programs that divide computation work
into a number of threads. Grace provides the highest scalability
when these threads are reasonably long-lived and do not modify much
data that is shared across the threads.

# Building Grace #

Just change into the `src/` directory and type `make gcc-x86`.

Grace currently works only on x86-based Linux platforms with the GNU
C/C++ compilers.

# Using Grace #

To use Grace, change the final compile step in your program (the
link stage) so that it links with Grace instead of with pthreads.

For example, change this link step:

	g++ mycode.cpp -lpthreads -o mycode

to the following:

	g++ -Wl,-T grace.ld mycode.cpp -L/grace/install/dir -lgrace -o mycode

where `/grace/install/dir` should be replaced by wherever you choose
to install Grace.

*NOTE:* Grace does not yet work in 64-bit mode. When compiling code on
64-bit architectures with GNU C++, use the `-m32` compiler flag, as
in:
  
	g++ -m32 -Wl,-T grace.ld mycode.cpp -L/grace/install/dir -lgrace -o mycode

# Limitations #

* Grace currently is only supported for 32-bit, x86 platforms;
  Linux / GNU only.

* Grace can *significantly* slow down a multithreaded
  program. Your mileage may vary. Grace will scale perfectly for a
  program whose threads do not update any data shared across the
  threads. Detailed experiments measuring the relative effect of
  different thread lengths (in runtime) and amount of sharing are in
  our paper, listed below.

* Grace is currently designed to support programs that use multiple
  threads *to speed up computation* using *fork-join parallelism*. In
  particular, Grace does not currently support threads that "run
  forever" or that communicate with each other via condition
  variables. Dthreads, our follow-on project, handles such programs.

# More information #

Grace consists of several parts: (1) a novel VIRTUAL MEMORY BASED
SOFTWARE TRANSACTIONAL MEMORY system (STM) implemented on top of
processes, (2) an ORDERING PROTOCOL to guarantee sequential program
semantics, and (3) an OPTIMIZED MMAP-BASED HEAP AND GLOBAL VARIABLES
that lets processes behave like threads.

For more detailed technical information about Grace, see the
following paper:

[Grace: Safe Multithreaded Programming for C/C++](http://people.cs.umass.edu/~emery/pubs/grace-oopsla09.pdf)
Emery D. Berger, Ting Yang, Tongping Liu, and Gene Novark.
OOPSLA 2009

(included as doc/grace-oopsla09.pdf)









  