#ifndef __THREAD__
#define __THREAD__

#include <pthread.h>

#include "java/lang/Runnable.h"
#include "java/lang/String.h"

namespace java {

/**
Emulation of `java.lang.Thread` over POSIX threads: runs the `run` method of
its target in a new thread of execution, once `start` is called.

As in Java, a started thread keeps running when the object is destroyed
(it is detached), so a thread may even destroy its own `Thread` object.
The target is not owned.
*/
class Thread {
  private:
    Runnable *target;
    java::String name;
    pthread_t thread;
    bool joinable;

    Thread(const Thread &other);
    Thread &operator=(const Thread &other);

  public:
    explicit Thread(Runnable *target);
    ~Thread();

    void
    setName(const java::String &name);

    const java::String &
    getName() const;

    /**
    Starts the execution of `target->run()` in a new thread.
    */
    void
    start();

    /**
    Waits for the end of the thread, if it was started and not joined.
    */
    void
    join();

    /**
    @param millis time to wait, in milliseconds
    */
    static void
    sleep(long long millis);
};

}

#endif
