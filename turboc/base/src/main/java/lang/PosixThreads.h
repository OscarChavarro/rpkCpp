#ifndef __POSIX_THREADS__
#define __POSIX_THREADS__

/**
POSIX threads or, when VITRAL_WITHOUT_PTHREADS is defined, a single threaded
replacement of the subset used by the port, for legacy systems without a
usable pthreads library (i.e. Debian 1.1 or DOS): mutexes and condition
variables do nothing (there is no other thread to wait for) and
`pthread_create` fails, so no thread is ever started.

The legacy Makefiles define VITRAL_WITHOUT_PTHREADS for the whole build
(library and programs) unless WITH_POSIX_THREADS is given, as the layout
of the classes that hold mutexes depends on it.
*/
#ifndef VITRAL_WITHOUT_PTHREADS
#include <pthread.h>
#else

// System types first: some C libraries declare the pthread types there,
// and those declarations must be done before the names are redefined below
#include <sys/types.h>

struct VitralNoThreadsMutex {
    int unused;
};

struct VitralNoThreadsMutexAttributes {
    int type;
};

struct VitralNoThreadsCondition {
    int unused;
};

typedef unsigned long VitralNoThreadsThread;

#define pthread_t VitralNoThreadsThread
#define pthread_mutex_t VitralNoThreadsMutex
#define pthread_mutexattr_t VitralNoThreadsMutexAttributes
#define pthread_cond_t VitralNoThreadsCondition
#define PTHREAD_MUTEX_RECURSIVE 1

inline int vitralNoThreadsMutexInit(VitralNoThreadsMutex *,
    const VitralNoThreadsMutexAttributes *) { return 0; }
inline int vitralNoThreadsMutexOperation(VitralNoThreadsMutex *)
    { return 0; }
inline int vitralNoThreadsMutexAttributesInit(
    VitralNoThreadsMutexAttributes *attributes)
    { attributes->type = 0; return 0; }
inline int vitralNoThreadsMutexAttributesSetType(
    VitralNoThreadsMutexAttributes *attributes, int type)
    { attributes->type = type; return 0; }
inline int vitralNoThreadsMutexAttributesDestroy(
    VitralNoThreadsMutexAttributes *) { return 0; }
inline int vitralNoThreadsConditionInit(VitralNoThreadsCondition *,
    const void *) { return 0; }
inline int vitralNoThreadsConditionOperation(VitralNoThreadsCondition *)
    { return 0; }
inline int vitralNoThreadsConditionWait(VitralNoThreadsCondition *,
    VitralNoThreadsMutex *) { return 0; }
inline int vitralNoThreadsCreate(VitralNoThreadsThread *, const void *,
    void *(*)(void *), void *) { return -1; }
inline int vitralNoThreadsJoin(VitralNoThreadsThread, void **)
    { return 0; }
inline int vitralNoThreadsDetach(VitralNoThreadsThread) { return 0; }
inline VitralNoThreadsThread vitralNoThreadsSelf() { return 0; }
inline int vitralNoThreadsEqual(VitralNoThreadsThread thread1,
    VitralNoThreadsThread thread2) { return thread1 == thread2; }

#define pthread_mutex_init vitralNoThreadsMutexInit
#define pthread_mutex_destroy vitralNoThreadsMutexOperation
#define pthread_mutex_lock vitralNoThreadsMutexOperation
#define pthread_mutex_unlock vitralNoThreadsMutexOperation
#define pthread_mutexattr_init vitralNoThreadsMutexAttributesInit
#define pthread_mutexattr_settype vitralNoThreadsMutexAttributesSetType
#define pthread_mutexattr_destroy vitralNoThreadsMutexAttributesDestroy
#define pthread_cond_init vitralNoThreadsConditionInit
#define pthread_cond_destroy vitralNoThreadsConditionOperation
#define pthread_cond_wait vitralNoThreadsConditionWait
#define pthread_cond_signal vitralNoThreadsConditionOperation
#define pthread_cond_broadcast vitralNoThreadsConditionOperation
#define pthread_create vitralNoThreadsCreate
#define pthread_join vitralNoThreadsJoin
#define pthread_detach vitralNoThreadsDetach
#define pthread_self vitralNoThreadsSelf
#define pthread_equal vitralNoThreadsEqual

#endif

#endif
