#ifndef __JAVA_CONDITION__
#define __JAVA_CONDITION__

#include <pthread.h>

#include "java/util/concurrent/locks/ReentrantLock.h"

namespace java {

/**
Emulation of `java.util.concurrent.locks.Condition`, as given by
`ReentrantLock.newCondition()`: it is built over its lock. As in Java, the
lock must be held (once) when calling `await`, `signal` or `signalAll`.
*/
class Condition {
  private:
    ReentrantLock &lock;
    pthread_cond_t condition;

  public:
    explicit Condition(ReentrantLock &inLock) : lock(inLock)
    {
        pthread_cond_init(&condition, nullptr);
    }

    Condition(const Condition &) = delete;
    Condition &operator=(const Condition &) = delete;

    ~Condition()
    {
        pthread_cond_destroy(&condition);
    }

    /**
    Releases the lock until signalled (or spuriously woken up, as Java
    allows), then takes it again
    */
    void
    await()
    {
        pthread_cond_wait(&condition, lock.nativeMutex());
    }

    void
    signal()
    {
        pthread_cond_signal(&condition);
    }

    void
    signalAll()
    {
        pthread_cond_broadcast(&condition);
    }
};

}

#endif
