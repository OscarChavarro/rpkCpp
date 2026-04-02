#ifndef __CORE_OPTIONS_PARSER__
#define __CORE_OPTIONS_PARSER__

class Background;
class ParseSession;
class Scene;
class RenderOptions;
class ToneMappingContext;
class OptionsType;

class CoreOptionsParser final {
  public:
    static void parse(
        int *argc,
        char **argv,
        ParseSession &parseSession,
        Scene &scene,
        RenderOptions &renderOptions,
        ToneMappingContext &toneMapOptions,
        int &imageOutputWidth,
        int &imageOutputHeight,
        bool &glutDebugEnabled,
        char *toneMapNameOut,
        OptionsType &optionTypes);

    static Background *createBackground();
};

#endif
