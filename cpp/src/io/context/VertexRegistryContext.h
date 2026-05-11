#ifndef VERTEX_REPOSITORY__
#define VERTEX_REPOSITORY__

#include "common/dataStructures/LookUpTable.h"
#include "io/context/VertexContext.h"

class VertexRegistryContext {
  public:
    LookUpTable<char *> *vertexLookUpTable;
    VertexContext defaultVertexContext;
    VertexContext unNamedVertexContext;
    VertexContext *currentVertex;

    VertexRegistryContext();
    ~VertexRegistryContext();

    void reset();

    VertexRegistryContext(const VertexRegistryContext &) = delete;
    VertexRegistryContext &operator=(const VertexRegistryContext &) = delete;

  private:
    static const Vector3Dd ZERO_VECTOR;
};

#endif
