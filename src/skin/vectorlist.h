#ifndef __Vector3DListNode__
#define __Vector3DListNode__

#include "common/linealAlgebra/vectorMacros.h"
#include "common/dataStructures/List.h"

class Vector3DListNode {
  public:
    Vector3D *vector;
    Vector3DListNode *next;
};

#define VectorListAdd(vectorlist, vector) \
    (Vector3DListNode *)ListAdd((LIST *)vectorlist, (void *)vector)
#endif
