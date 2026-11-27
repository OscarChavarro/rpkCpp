#ifndef __OPTIONS_GROUP_RAYTRACING__
#define __OPTIONS_GROUP_RAYTRACING__

#include "vsdk/toolkit/common/VSDK.h"

class OptionsGroupRaytracing{ public:
    static void parse( int *argc, char **argv, char *rayTracerName);

  private:
    static void makeMethodsHelpMessage(char *buffer);
};

#endif
