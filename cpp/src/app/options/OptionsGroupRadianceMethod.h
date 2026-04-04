#ifndef __COMMAND_LINE_RADIANCE_METHOD_OPTIONS_GROUP__
#define __COMMAND_LINE_RADIANCE_METHOD_OPTIONS_GROUP__

class OptionsGroupRadianceMethod final {
  public:
    static void radianceMethodParseOptions(
        int *argc,
        char **argv,
        char *radianceMethodsStringOut);
};

#endif
