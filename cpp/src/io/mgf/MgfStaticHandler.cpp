#include "io/context/ParseErrorContext.h"
#include "io/mgf/MgfStaticHandler.h"

MgfStaticHandler::MgfStaticHandler(
    const HandlerRoleContext handlerType,
    const HandlerFunction handlerFunction):
    handlerType(handlerType),
    handlerFunction(handlerFunction)
{
}

int
MgfStaticHandler::handle(int argc, const char **argv, ParseContext *context) const {
    if ( handlerFunction == nullptr ) {
        return ParseErrorContext::MGF_OK;
    }
    return handlerFunction(argc, argv, static_cast<ParseRuntimeContext *>(context));
}

HandlerRoleContext
MgfStaticHandler::type() const {
    return handlerType;
}
