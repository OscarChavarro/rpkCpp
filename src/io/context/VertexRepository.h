#ifndef __VERTEX_REPOSITORY__
#define __VERTEX_REPOSITORY__

#include "io/context/LookUpTable.h"
#include "io/mgf/MgfVertexContext.h"

class VertexRepository {
  public:
    LookUpTable *vertexLookUpTable;
    MgfVertexContext defaultVertexContext;
    MgfVertexContext unNamedVertexContext;
    MgfVertexContext *currentVertex;

    VertexRepository();
    ~VertexRepository();

    void reset();

    VertexRepository(const VertexRepository &) = delete;
    VertexRepository &operator=(const VertexRepository &) = delete;
};

#endif
