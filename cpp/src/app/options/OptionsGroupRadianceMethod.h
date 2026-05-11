#ifndef COMMAND_LINE_RADIANCE_METHOD_OPTIONS_GROUP__
#define COMMAND_LINE_RADIANCE_METHOD_OPTIONS_GROUP__

class OptionsGroupRadianceMethod final {
  public:
    static void radianceMethodParseOptions(
        int *argc,
        char **argv,
        char *radianceMethodsStringOut);
};

#endif
