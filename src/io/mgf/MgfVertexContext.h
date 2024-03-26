#ifndef __MGF_VERTEX_CONTEXT__
#define __MGF_VERTEX_CONTEXT__

#include "common/linealAlgebra/Vector3Dd.h"

class MgfVertexContext {
  public:
    VECTOR3Dd p; // Point
    VECTOR3Dd n; // Normal
    long xid; // Transform id of transform last time the vertex was modified (or created)
    int clock; // Incremented each change -- resettable
    void *clientData; // Client data -- initialized to nullptr by the parser
};

extern MgfVertexContext *getNamedVertex(char *name, MgfContext *context);

#endif
