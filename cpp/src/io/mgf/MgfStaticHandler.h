#ifndef __MGF_STATIC_HANDLER__
#define __MGF_STATIC_HANDLER__

#include "io/context/ParseSession.h"
#include "io/context/EntityHandler.h"

class MgfStaticHandler final : public EntityHandler {
  public:
    using HandlerFunction = int (*)(int argc, const char **argv, ParseSession *context);

    MgfStaticHandler(HandlerType handlerType, HandlerFunction handlerFunction);

    int handle(int argc, const char **argv, ParseContext *context) const override;
    HandlerType type() const override;

  private:
    HandlerType handlerType;
    HandlerFunction handlerFunction;
};

#endif
