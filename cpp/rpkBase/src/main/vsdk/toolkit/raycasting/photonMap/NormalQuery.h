#ifndef __NORMAL_QUERY__
#define __NORMAL_QUERY__

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/raycasting/photonMap/IrrPhoton.h"

class NormalQuery {
  public:
    IrrPhoton *photon;
    Vector3Dd point;
    Vector3Dd normal;
    double threshold;
    double maximumDistance;

    NormalQuery();
};

#endif
