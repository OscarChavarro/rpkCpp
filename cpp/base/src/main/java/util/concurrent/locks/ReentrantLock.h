#ifndef __JAVA_REENTRANT_LOCK__
#define __JAVA_REENTRANT_LOCK__

#include <pthread.h>

namespace java {

/**
Emulation of `java.util.concurrent.locks.ReentrantLock` over a recursive
POSIX mutex.
*/
class ReentrantLock {
  private:
    pthread_mutex_t mutex;

  public:
    ReentrantLock()
    {
        pthread_mutexattr_t attributes;
        pthread_mutexattr_init(&attributes);
        pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&mutex, &attributes);
        pthread_mutexattr_destroy(&attributes);
    }

    ReentrantLock(const ReentrantLock &) = delete;
    ReentrantLock &operator=(const ReentrantLock &) = delete;

    ~ReentrantLock()
    {
        pthread_mutex_destroy(&mutex);
    }

    void
    lock()
    {
        pthread_mutex_lock(&mutex);
    }

    void
    unlock()
    {
        pthread_mutex_unlock(&mutex);
    }

    /**
    @return the native mutex, for the `Condition` objects of this lock
    */
    pthread_mutex_t *
    nativeMutex()
    {
        return &mutex;
    }
};

/**
Holds a `ReentrantLock` during its lifetime, as the `synchronized` blocks
of Java.
*/
class ReentrantLockGuard {
  private:
    ReentrantLock &lock;

  public:
    explicit ReentrantLockGuard(ReentrantLock &inLock) : lock(inLock)
    {
        lock.lock();
    }

    ReentrantLockGuard(const ReentrantLockGuard &) = delete;
    ReentrantLockGuard &operator=(const ReentrantLockGuard &) = delete;

    ~ReentrantLockGuard()
    {
        lock.unlock();
    }
};

}

#endif
