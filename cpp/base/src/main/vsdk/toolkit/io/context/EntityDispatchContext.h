#ifndef MGF_ENTITY_HANDLER__
#define MGF_ENTITY_HANDLER__

#include "vsdk/toolkit/io/context/HandlerRoleContext.h"
#include "vsdk/toolkit/io/context/ParseContext.h"

class EntityDispatchContext {
  public:
    virtual int handle(int argc, const char **argv, ParseContext *context) const = 0;
    virtual HandlerRoleContext type() const = 0;
    virtual ~EntityDispatchContext() {}
};

#endif
