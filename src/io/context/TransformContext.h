#ifndef __TRANSFORM_CONTEXT__
#define __TRANSFORM_CONTEXT__

#include "io/mgf/MgfTransform.h"
#include "io/context/TransformArray.h"

class TransformContext {
  public:
    long xid; // Unique transform id
    short xac; // Context argument count
    short rev; // Boolean true if vertices reversed
    short ownedArgumentCount; // Number of owned argument copies
    MgfTransform xf; // Cumulative transformation
    TransformArray *transformationArray;
    char **ownedArgumentCopies; // Copies for non-iterative transform arguments
    TransformContext *prev; // Previous transformation context

    TransformContext() :
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
