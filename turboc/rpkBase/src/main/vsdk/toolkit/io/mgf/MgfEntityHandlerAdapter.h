#ifndef __MGF_ENTITY_HANDLER_ADAPTER__
#define __MGF_ENTITY_HANDLER_ADAPTER__

#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"
#include "vsdk/toolkit/io/context/EntityDispatchContext.h"

class MgfEntityHandlerAdapter: public EntityDispatchContext{ public:
    typedef int (*HandlerFunction)(int argc, const char **argv, ParseRuntimeContext *context);

    MgfEntityHandlerAdapter(HandlerRoleContext handlerType, HandlerFunction handlerFunction);

    int handle(int argc, const char **argv, ParseContext *context) const;
    HandlerRoleContext type() const;

  private:
    HandlerRoleContext handlerType;
    HandlerFunction handlerFunction;
};

#endif
