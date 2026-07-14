#ifndef TRANSFORM_STACK_CONTEXT__
#define TRANSFORM_STACK_CONTEXT__

#include "vsdk/toolkit/io/context/TransformSequenceContext.h"
#include "vsdk/toolkit/io/context/TransformContext.h"

class TransformStackContext {
  public:
    long xid; // Unique transform id
    short xac; // Context argument count
    short rev; // Boolean true if vertices reversed
    short ownedArgumentCount; // Number of owned argument copies
    TransformContext xf; // Cumulative transformation
    TransformSequenceContext *transformationArray;
    char **ownedArgumentCopies; // Copies for non-iterative transform arguments
    TransformStackContext *prev; // Previous transformation context

    TransformStackContext() :
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
