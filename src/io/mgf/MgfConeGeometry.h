#ifndef __MGF_CONE_GEOMETRY__
#define __MGF_CONE_GEOMETRY__

#include "io/context/ParseSession.h"

class MgfConeGeometry final {
  public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseSession *context);
};

#endif
