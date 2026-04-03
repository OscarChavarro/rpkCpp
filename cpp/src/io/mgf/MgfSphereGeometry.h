#ifndef __MGF_SPHERE_GEOMETRY__
#define __MGF_SPHERE_GEOMETRY__

#include "io/context/ParseSession.h"

class MgfSphereGeometry final {
  public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseSession *context);
};

#endif
