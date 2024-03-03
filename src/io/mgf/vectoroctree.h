#ifndef __VECTOR_OCTREE__
#define __VECTOR_OCTREE__

#include "common/linealAlgebra/Vector3D.h"

class VectorOctreeNode {
  public:
    Vector3D *vector;
    VectorOctreeNode *child[8];
};

#endif
