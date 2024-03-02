#ifndef __VECTOR_OCTREE__
#define __VECTOR_OCTREE__

#include "common/linealAlgebra/Vector3D.h"

class VECTOROCTREE {
  public:
    Vector3D *vector;
    VECTOROCTREE *child[8];
};

#endif
