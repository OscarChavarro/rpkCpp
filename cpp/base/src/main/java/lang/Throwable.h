#ifndef __JAVA_THROWABLE__
#define __JAVA_THROWABLE__

#include "java/lang/Object.h"
#include "java/lang/String.h"

namespace java {

/**
Emulation of `java.lang.Throwable`, root of the exceptions thrown by the
port.
*/
class Throwable : public Object {
  private:
    String message;

  public:
    Throwable() : message("") {}
    explicit Throwable(const String &message) : message(message) {}
    Throwable(const Throwable &other) : Object(), message(other.message) {}
    virtual ~Throwable() {}

    /**
    @return the message given at construction time
    */
    const String &
    getMessage() const
    {
        return message;
    }
};

}

#endif
