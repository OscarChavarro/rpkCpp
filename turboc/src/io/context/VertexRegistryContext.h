#ifndef __VERTEX_REPOSITORY__
#define __VERTEX_REPOSITORY__

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

    VertexRegistryContext(const VertexRegistryContext &);
    VertexRegistryContext &operator=(const VertexRegistryContext &);

  private:
    static const Vector3Dd ZERO_VECTOR;
};

#endif
