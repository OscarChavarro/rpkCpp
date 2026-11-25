#ifndef __OPTIONS_GROUP_RADIANCE_METHOD__
#define __OPTIONS_GROUP_RADIANCE_METHOD__

class OptionsGroupRadianceMethod final {
  public:
    static void radianceMethodParseOptions(
        int *argc,
        char **argv,
        char *radianceMethodsStringOut);
};

#endif
