C library for concurrency task system
=====================================

Introduction
------------

This is the ...

Concept
-------

- task control block support. (name, and more)
- suspend / resume support.
- thread pool support.
- channel support. (inter task communication)
- stack tracing support. (maybe)

Dependencies
------------

- C11.
- Posix threads.

Reference
---------

- [Simple, Fast, and Practical Non-Blocking and Blocking Concurrent Queue Algorithms](http://www.cs.rochester.edu/u/scott/papers/1996_PODC_queues.pdf)
- [Lock-Free Linked Lists and Skip Lists](http://www.cse.yorku.ca/~ruppert/papers/lfll.pdf)
- [Writing a portable lock-free reader/writer lock](https://yizhang82.dev/lock-free-rw-lock)
- https://nullprogram.com/blog/2014/09/02/
- https://www.slideshare.net/kumagi/lock-free-safe
- https://github.com/s-hironobu/AlgorithmCollection
- https://github.com/yamasa/lockfree
- https://github.com/darkautism/lfqueue
