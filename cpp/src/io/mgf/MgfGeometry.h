#ifndef __MGF_GEOMETRY__
#define __MGF_GEOMETRY__

#include "common/linealAlgebra/Vector3Dd.h"

class MgfGeometry {
  public:
    static constexpr int MGF_PV_SIZE = 24;

    static void formatFloat(char *target, int targetLength, double value);
    static void mgfMakeAxes(Vector3Dd *u, Vector3Dd *v, const Vector3Dd *w, double epsilon);
};

#endif
