#ifndef __NORMAL_QUERY__
#define __NORMAL_QUERY__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"
#include "vsdk/toolkit/raycasting/photonMap/IrrPhoton.h"

class NormalQuery {
  public:
    IrrPhoton *photon;
    float *point;
    Vector3Df normal;
    float threshold;
    float maximumDistance;

    NormalQuery();
};

#endif
