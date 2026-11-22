#include <ctime>

#include "java/lang/Thread.h"

namespace java {

namespace {

/**
Startup data of a new thread: the thread waits until `started` is unlocked,
i.e. until `Thread::start` knows it, as its task may destroy the `Thread`.
*/
struct ThreadStartup {
    Runnable *task;
    pthread_mutex_t started;
};

void *
runThreadStartup(void *data)
{
    ThreadStartup *startup = static_cast<ThreadStartup *>(data);
    pthread_mutex_lock(&startup->started);
    pthread_mutex_unlock(&startup->started);
    Runnable *task = startup->task;
    pthread_mutex_destroy(&startup->started);
    delete startup;
    if ( task != nullptr ) {
        task->run();
    }
    return nullptr;
}

}

Thread::Thread(Runnable *target) : target(target), name("Thread"), thread(), joinable(false)
{
}

Thread::~Thread()
{
    if ( joinable ) {
        pthread_detach(thread);
        joinable = false;
    }
}

void
Thread::setName(const java::String &name)
{
    this->name = name;
}

const java::String &
Thread::getName() const
{
    return name;
}

void
Thread::start()
{
    // The new thread waits until this object knows it: then its task may
    // destroy this object
    ThreadStartup *startup = new ThreadStartup();
    startup->task = target;
    pthread_mutex_init(&startup->started, nullptr);
    pthread_mutex_lock(&startup->started);
    pthread_t newThread;
    if ( pthread_create(&newThread, nullptr, runThreadStartup, startup) != 0 ) {
        pthread_mutex_unlock(&startup->started);
        pthread_mutex_destroy(&startup->started);
        delete startup;
        return;
    }
    thread = newThread;
    joinable = true;
    pthread_mutex_unlock(&startup->started);
}

void
Thread::join()
{
    if ( joinable && !pthread_equal(thread, pthread_self()) ) {
        pthread_join(thread, nullptr);
        joinable = false;
    }
}

void
Thread::sleep(long long millis)
{
    if ( millis <= 0 ) {
        return;
    }
    struct timespec duration;
    duration.tv_sec = static_cast<time_t>(millis / 1000);
    duration.tv_nsec = static_cast<long>((millis % 1000) * 1000000);
    while ( nanosleep(&duration, &duration) != 0 ) {
    }
}

}
