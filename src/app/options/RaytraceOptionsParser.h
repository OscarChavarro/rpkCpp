#ifndef __RAYTRACE_OPTIONS_PARSER__
#define __RAYTRACE_OPTIONS_PARSER__

#include "app/options/OptionsType.h"

class RaytraceOptionsParser final {
  public:
    static void parse(
        int *argc,
        char **argv,
        char *rayTracerName,
        OptionsType &optionTypes);

  private:
    static void makeMethodsHelpMessage(char *buffer);
};

#endif
