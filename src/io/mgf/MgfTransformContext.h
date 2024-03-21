#ifndef __MGF_TRANSFORM_CONTEXT__
#define __MGF_TRANSFORM_CONTEXT__

#include "io/mgf/parser.h"

class MgfTransformContext {
public:
    long xid; // Unique transform id
    short xac; // Context argument count
    short rev; // Boolean true if vertices reversed
    XF xf; // Cumulative transformation
    MgfTransformArray *xarr; // Transformation array pointer
    MgfTransformContext *prev; // Previous transformation context
}; // Followed by argument buffer

extern MgfTransformContext *GLOBAL_mgf_xfContext; // Current transform context

#endif
