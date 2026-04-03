#ifndef __VERTEX_REPOSITORY__
#define __VERTEX_REPOSITORY__

#include "io/context/LookUpTable.h"
#include "io/context/VertexContext.h"

class VertexRepository {
  public:
    LookUpTable<char *> *vertexLookUpTable;
    VertexContext defaultVertexContext;
    VertexContext unNamedVertexContext;
    VertexContext *currentVertex;

    VertexRepository();
    ~VertexRepository();

    void reset();

    VertexRepository(const VertexRepository &) = delete;
    VertexRepository &operator=(const VertexRepository &) = delete;

  private:
    static const Vector3Dd ZERO_VECTOR;
};

#endif
