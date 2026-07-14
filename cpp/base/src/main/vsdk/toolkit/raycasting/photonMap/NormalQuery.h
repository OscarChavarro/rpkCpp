#ifndef NORMAL_QUERY__
#define NORMAL_QUERY__

#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"
#include "vsdk/toolkit/raycasting/photonMap/IrrPhoton.h"

class NormalQuery {
  public:
    IrrPhoton *photon;
    float *point;
    Vector3D normal;
    float threshold;
    float maximumDistance;

    NormalQuery();
};

#endif
