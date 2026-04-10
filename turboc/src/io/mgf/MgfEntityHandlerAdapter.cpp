#include "io/context/ParseErrorContext.h"
#include "io/mgf/MgfEntityHandlerAdapter.h"

MgfEntityHandlerAdapter::MgfEntityHandlerAdapter(
    const HandlerRoleContext handlerType,
    const HandlerFunction handlerFunction):
    handlerType(handlerType),
    handlerFunction(handlerFunction)
{
}

int
MgfEntityHandlerAdapter::handle(int argc, const char **argv, ParseContext *context) const {
    if ( handlerFunction == NULL ) {
        return MGF_OK;
    }
    return handlerFunction(argc, argv, ((ParseRuntimeContext *)(context)));
}

HandlerRoleContext
MgfEntityHandlerAdapter::type() const {
    return handlerType;
}
