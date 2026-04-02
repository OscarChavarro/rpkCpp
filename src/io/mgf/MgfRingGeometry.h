#ifndef __MGF_RING_GEOMETRY__
#define __MGF_RING_GEOMETRY__

#include "io/context/ParseSession.h"

class MgfRingGeometry final {
  public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseSession *context);
};

#endif
