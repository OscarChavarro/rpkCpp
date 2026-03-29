#ifndef __MGF_VERTEX_CONTEXT__
#define __MGF_VERTEX_CONTEXT__

#include "common/linealAlgebra/Vector3Dd.h"
#include "skin/Vertex.h"

class MgfParseSession;

class MgfVertexContext {
  public:
    Vector3Dd p; // Point
    Vector3Dd n; // Normal
    long xid; // Transform id of transform last time the vertex was modified (or created)
    int clock; // Incremented each change -- resettable
    Vertex *vertex;

    MgfVertexContext():
        p(), n(), xid(), clock(), vertex() {
    }

    MgfVertexContext(const Vector3Dd &inP, const Vector3Dd &inN, long inXid, int inClock, Vertex *inVertex):
        p(), n(), xid(), clock(), vertex() {
        p = inP;
        n = inN;
        xid = inXid;
        clock = inClock;
        vertex = inVertex;
    };
};

extern MgfVertexContext *getNamedVertex(const char *name, MgfParseSession *context);

#endif
