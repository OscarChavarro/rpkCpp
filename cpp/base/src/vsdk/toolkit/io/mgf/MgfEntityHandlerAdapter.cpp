#include "vsdk/toolkit/io/context/ParseErrorContext.h"
#include "vsdk/toolkit/io/mgf/MgfEntityHandlerAdapter.h"

MgfEntityHandlerAdapter::MgfEntityHandlerAdapter(
    const HandlerRoleContext handlerType,
    const HandlerFunction handlerFunction):
    handlerType(handlerType),
    handlerFunction(handlerFunction)
{
}

int
MgfEntityHandlerAdapter::handle(int argc, const char **argv, ParseContext *context) const {
    if ( handlerFunction == nullptr ) {
        return ParseErrorContext::MGF_OK;
    }
    return handlerFunction(argc, argv, static_cast<ParseRuntimeContext *>(context));
}

HandlerRoleContext
MgfEntityHandlerAdapter::type() const {
    return handlerType;
}
