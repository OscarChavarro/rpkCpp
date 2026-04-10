#ifndef __RAYTRACING_OPTIONS_GROUP__
#define __RAYTRACING_OPTIONS_GROUP__

#include "common/VSDK.h"

class OptionsGroupRaytracing{ public:
    static void parse( int *argc, char **argv, char *rayTracerName);

  private:
    static void makeMethodsHelpMessage(char *buffer);
};

#endif
