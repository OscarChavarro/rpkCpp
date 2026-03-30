#ifndef __MGF_ENTITY_HANDLER__
#define __MGF_ENTITY_HANDLER__

#include "io/context/ParseSession.h"
#include "HandlerType.h"

class EntityHandler {
  public:
    virtual int handle(int argc, const char **argv, ParseSession *context) const = 0;
    virtual HandlerType type() const = 0;
    virtual ~EntityHandler() {}
};

#endif
