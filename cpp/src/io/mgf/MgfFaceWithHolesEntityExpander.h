#ifndef __MGF_FACE_WITH_HOLES_GEOMETRY__
#define __MGF_FACE_WITH_HOLES_GEOMETRY__

#include "io/context/ParseRuntimeContext.h"

class MgfFaceWithHolesEntityExpander final {
  public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
};

#endif
