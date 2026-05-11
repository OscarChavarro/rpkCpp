#ifndef MGF_FACE_WITH_HOLES_GEOMETRY__
#define MGF_FACE_WITH_HOLES_GEOMETRY__

#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"

class MgfFaceWithHolesEntityExpander final {
  public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
};

#endif
