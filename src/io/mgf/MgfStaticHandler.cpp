#include "io/context/ErrorCodeContext.h"
#include "io/mgf/MgfStaticHandler.h"

MgfStaticHandler::MgfStaticHandler(
    const MgfHandlerType handlerType,
    const HandlerFunction handlerFunction):
    handlerType(handlerType),
    handlerFunction(handlerFunction)
{
}

int
MgfStaticHandler::handle(int argc, const char **argv, MgfParseSession *context) const {
    if ( handlerFunction == nullptr ) {
        return ErrorCodeContext::MGF_OK;
    }
    return handlerFunction(argc, argv, context);
}

MgfHandlerType
MgfStaticHandler::type() const {
    return handlerType;
}
