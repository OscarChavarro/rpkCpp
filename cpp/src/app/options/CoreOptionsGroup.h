#ifndef __CORE_OPTIONS_GROUP__
#define __CORE_OPTIONS_GROUP__

#include "common/RenderOptions.h"
#include "io/context/ParseRuntimeContext.h"
#include "scene/Background.h"
#include "scene/Scene.h"
#include "tonemap/ToneMappingContext.h"

class CoreOptionsGroup final {
  public:
    static void parse(
        int *argc,
        char **argv,
        ParseRuntimeContext &parseSession,
        Scene &scene,
        RenderOptions &renderOptions,
        ToneMappingContext &toneMapOptions,
        int &imageOutputWidth,
        int &imageOutputHeight,
        bool &glutDebugEnabled,
        char *toneMapNameOut);

    static Background *createBackground();
};

#endif
