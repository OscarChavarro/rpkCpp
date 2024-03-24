#ifndef __MGF_CONTEXT__
#define __MGF_CONTEXT__

#include "skin/RadianceMethod.h"

class MgfContext {
  public:
    // Parameters received from main program
    RadianceMethod *radianceMethod;
    bool singleSided;
    char *currentVertexName;

    // Global variables on the MGF reader context

    // Return model
};

#endif
