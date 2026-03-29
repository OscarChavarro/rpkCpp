#ifndef __MGF_STATIC_HANDLER__
#define __MGF_STATIC_HANDLER__

#include "io/mgf/MgfEntityHandler.h"

class MgfStaticHandler final : public MgfEntityHandler {
  public:
    using HandlerFunction = int (*)(int argc, const char **argv, MgfParseSession *context);

    MgfStaticHandler(MgfHandlerType handlerType, HandlerFunction handlerFunction);

    int handle(int argc, const char **argv, MgfParseSession *context) const override;
    MgfHandlerType type() const override;

  private:
    MgfHandlerType handlerType;
    HandlerFunction handlerFunction;
};

#endif
