#ifndef __MGF_TRANSFORM_CONTEXT__
#define __MGF_TRANSFORM_CONTEXT__

#include "io/mgf/MgfTransform.h"
#include "io/mgf/MgfTransformArray.h"

class MgfTransformContext {
  public:
    long xid; // Unique transform id
    short xac; // Context argument count
    short rev; // Boolean true if vertices reversed
    MgfTransform xf; // Cumulative transformation
    MgfTransformArray *transformationArray;
    MgfTransformContext *prev; // Previous transformation context

    MgfTransformContext() : xid(), xac(), rev(), xf(), transformationArray(), prev() {};
}; // Followed by argument buffer

#endif
