#ifndef __MGF_ENTITY_HANDLER__
#define __MGF_ENTITY_HANDLER__

#include "io/context/MgfParseSession.h"
#include "io/mgf/MgfHandlerType.h"

class MgfEntityHandler {
  public:
    virtual int handle(int argc, const char **argv, MgfParseSession *context) const = 0;
    virtual MgfHandlerType type() const = 0;
    virtual ~MgfEntityHandler() {}
};

#endif
