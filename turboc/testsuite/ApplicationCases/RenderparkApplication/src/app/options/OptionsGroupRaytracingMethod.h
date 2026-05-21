
#include "common/VSDK.h"
#ifndef CMMND_LINE_RYTRC_MTHD_OPTNS_GRP
#define CMMND_LINE_RYTRC_MTHD_OPTNS_GRP

class OptionsGroupRaytracingMethod{ public:
    static void rayTracingParseOptions( int *argc, char **argv, char raytracingMethodsStringOut[], char *rayTracerNameOut);

  private:
    static char *raytracingMethodsString;
    static char *rayTracerName;

    static void mainRayTracingOption(char *&value);
};

#endif
