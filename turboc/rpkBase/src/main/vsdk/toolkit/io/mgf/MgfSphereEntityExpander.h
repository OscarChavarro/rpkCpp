#ifndef __MGF_SPHERE_GEOMETRY__
#define __MGF_SPHERE_GEOMETRY__

#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"

class MgfSphereEntityExpander{ public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
};

#endif
