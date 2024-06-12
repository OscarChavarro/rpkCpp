#ifndef __MGF_VERTEX_CONTEXT__
#define __MGF_VERTEX_CONTEXT__

#include "common/linealAlgebra/Vector3Dd.h"

class MgfVertexContext {
  public:
    VECTOR3Dd p; // Point
    VECTOR3Dd n; // Normal
    long xid; // Transform id of transform last time the vertex was modified (or created)
    int clock; // Incremented each change -- resettable
    Vertex *vertex;

    MgfVertexContext():
        p(), n(), xid(), clock(), vertex() {
    }

    MgfVertexContext(const VECTOR3Dd &inP, const VECTOR3Dd &inN, long inXid, int inClock, Vertex *inVertex):
        p(), n(), xid(), clock(), vertex() {
        p = inP;
        n = inN;
        xid = inXid;
        clock = inClock;
        vertex = inVertex;
    };
};

extern MgfVertexContext *getNamedVertex(const char *name, MgfContext *context);

#endif
