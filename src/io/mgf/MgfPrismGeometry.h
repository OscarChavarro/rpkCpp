#ifndef __MGF_PRISM_GEOMETRY__
#define __MGF_PRISM_GEOMETRY__

#include "io/context/ParseSession.h"

class MgfPrismGeometry final {
  public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseSession *context);
};

#endif
