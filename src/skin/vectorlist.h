#ifndef __Vector3DListNode__
#define __Vector3DListNode__

#include "common/linealAlgebra/vectorMacros.h"
#include "common/dataStructures/List.h"

class Vector3DListNode {
  public:
    Vector3D *vector;
    Vector3DListNode *next;
};

#define VectorListCreate (Vector3DListNode *) ListCreate

#define VectorListAdd(vectorlist, vector) \
        (Vector3DListNode *)ListAdd((LIST *)vectorlist, (void *)vector)

#define VectorListIterate(vectorlist, proc) \
        ListIterate((LIST *)vectorlist, (void (*)(void *))proc)

#define VectorListDestroy(vectorlist) \
        ListDestroy((LIST *)vectorlist)

#endif
