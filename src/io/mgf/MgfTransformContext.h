#ifndef __MGF_TRANSFORM_CONTEXT__
#define __MGF_TRANSFORM_CONTEXT__

#include "io/mgf/MgfTransform.h"
#include "io/mgf/MgfTransformArray.h"

class MgfTransformContext {
  public:
    long xid; // Unique transform id
    short xac; // Context argument count
    short rev; // Boolean true if vertices reversed
    short ownedArgumentCount; // Number of owned argument copies
    MgfTransform xf; // Cumulative transformation
    MgfTransformArray *transformationArray;
    char **ownedArgumentCopies; // Copies for non-iterative transform arguments
    MgfTransformContext *prev; // Previous transformation context

    MgfTransformContext() :
        xid(),
        xac(),
        rev(),
        ownedArgumentCount(),
        xf(),
        transformationArray(),
        ownedArgumentCopies(),
        prev()
    {};
};

#endif
