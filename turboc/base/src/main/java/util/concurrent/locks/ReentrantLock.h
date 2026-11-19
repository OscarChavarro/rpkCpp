#ifndef __REENTRANT_LOCK__
#define __REENTRANT_LOCK__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/PosixThreads.h"


/**
Emulation of `java.util.concurrent.locks.ReentrantLock` over a recursive
POSIX mutex.
*/
class ReentrantLock {
  private:
    pthread_mutex_t mutex;

    ReentrantLock(const ReentrantLock &other);
    ReentrantLock &operator=(const ReentrantLock &other);

  public:
    ReentrantLock()
    {
        pthread_mutexattr_t attributes;
        pthread_mutexattr_init(&attributes);
        pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&mutex, &attributes);
        pthread_mutexattr_destroy(&attributes);
    }

    ~ReentrantLock()
    {
        pthread_mutex_destroy(&mutex);
    }

    void lock()
    {
        pthread_mutex_lock(&mutex);
    }

    void unlock()
    {
        pthread_mutex_unlock(&mutex);
    }
};

/**
Holds a `ReentrantLock` during its lifetime, as the `synchronized` blocks
of Java.
*/
class ReentrantLockGuard {
  private:
    ReentrantLock &lock;

    ReentrantLockGuard(const ReentrantLockGuard &other);
    ReentrantLockGuard &operator=(const ReentrantLockGuard &other);

  public:
    explicit ReentrantLockGuard(ReentrantLock &inLock) : lock(inLock)
    {
        lock.lock();
    }

    ~ReentrantLockGuard()
    {
        lock.unlock();
    }
};


#endif
