#ifndef __MGF_TORUS_GEOMETRY__
#define __MGF_TORUS_GEOMETRY__

#include "io/context/ParseSession.h"

class MgfTorusGeometry final {
  public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseSession *context);
};

#endif
