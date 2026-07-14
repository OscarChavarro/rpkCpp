#ifndef MGF_GEOMETRY__
#define MGF_GEOMETRY__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class MgfTessellationMath {
  public:
    static constexpr int MGF_PV_SIZE = 24;

    static void formatFloat(char *target, int targetLength, double value);
    static void mgfMakeAxes(Vector3Dd *u, Vector3Dd *v, const Vector3Dd *w, double epsilon);
};

#endif
