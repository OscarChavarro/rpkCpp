#ifndef __MGF_FACE_WITH_HOLES_ENTITY_EXPANDER__
#define __MGF_FACE_WITH_HOLES_ENTITY_EXPANDER__

#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"

class MgfFaceWithHolesEntityExpander final {
  public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
};

#endif
