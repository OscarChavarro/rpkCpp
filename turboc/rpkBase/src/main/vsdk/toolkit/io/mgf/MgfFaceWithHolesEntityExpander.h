#ifndef MGF_FACE_WITH_HLS_GMTRY
#define MGF_FACE_WITH_HLS_GMTRY

#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"

class MgfFaceWithHolesEntityExpander{ public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseRuntimeContext *context);
};

#endif
