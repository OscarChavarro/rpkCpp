/* vectoroctree.h: octrees containing vectors */

#ifndef _VECTOROCTREE_H_
#define _VECTOROCTREE_H_

#include "common/linealAlgebra/vectorMacros.h"
#include "common/dataStructures/Octree.h"

class VECTOROCTREE {
  public:
    Vector3D *vector;
    VECTOROCTREE *child[8];
};

#endif
