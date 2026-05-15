#ifndef __MGF_STATIC_HANDLER__
#define __MGF_STATIC_HANDLER__

#include "io/context/ParseRuntimeContext.h"
#include "io/context/EntityDispatchContext.h"

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
