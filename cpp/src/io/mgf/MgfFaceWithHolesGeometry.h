#ifndef __MGF_FACE_WITH_HOLES_GEOMETRY__
#define __MGF_FACE_WITH_HOLES_GEOMETRY__

#include "io/context/ParseSession.h"

class MgfFaceWithHolesGeometry final {
  public:
    static int handleEntity(int argumentCount, const char **argumentValues, ParseSession *context);
};

#endif
