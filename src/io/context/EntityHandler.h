#ifndef __MGF_ENTITY_HANDLER__
#define __MGF_ENTITY_HANDLER__

#include "io/context/HandlerType.h"
#include "io/context/ParseContext.h"

class EntityHandler {
  public:
    virtual int handle(int argc, const char **argv, ParseContext *context) const = 0;
    virtual HandlerType type() const = 0;
    virtual ~EntityHandler() {}
};

#endif
