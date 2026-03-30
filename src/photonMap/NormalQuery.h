#ifndef __NORMAL_QUERY__
#define __NORMAL_QUERY__

#include "common/linealAlgebra/Vector3D.h"
#include "photonMap/IrrPhoton.h"

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
