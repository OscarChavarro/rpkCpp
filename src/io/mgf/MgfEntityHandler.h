#ifndef __MGF_ENTITY_HANDLER__
#define __MGF_ENTITY_HANDLER__

#include "io/context/BaseContext.h"
#include "io/mgf/MgfHandlerType.h"

class MgfEntityHandler {
  public:
    virtual int handle(int argc, const char **argv, BaseContext *context) const = 0;
    virtual MgfHandlerType type() const = 0;
    virtual ~MgfEntityHandler() {}
};

#endif
