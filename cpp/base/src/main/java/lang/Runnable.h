#ifndef __RUNNABLE__
#define __RUNNABLE__

#include "java/util/function/_LambdaHolder.h"

namespace java {

/**
Emulation of `java.lang.Runnable`: a task, i.e. the body of a `Thread`.
As in Java, it is both an interface (subclasses override `run`) and the
target of a lambda (`java::Runnable task = [&]() { ... };`), whose
`run` calls it.
*/
class Runnable : public _Lambda<void> {
  public:
    Runnable() : _Lambda<void>() {}
    Runnable(decltype(nullptr)) : _Lambda<void>() {}
    Runnable(const Runnable &other) : _Lambda<void>(static_cast<const _Lambda<void> &>(other)) {}

    template <class F>
    Runnable(const F &function) : _Lambda<void>(function) {}

    Runnable &
    operator=(const Runnable &other)
    {
        _Lambda<void>::operator=(other);
        return *this;
    }

    virtual ~Runnable() {}

    virtual void
    run()
    {
        if ( static_cast<bool>(*this) ) {
            invoke();
        }
    }
};

}

#endif
