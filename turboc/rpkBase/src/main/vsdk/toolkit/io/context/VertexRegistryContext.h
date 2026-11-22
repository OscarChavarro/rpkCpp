#ifndef __VERTEX_REPOSITORY__
#define __VERTEX_REPOSITORY__

#include "vsdk/toolkit/common/dataStructures/LookUpTable.h"
#include "vsdk/toolkit/io/context/VertexContext.h"

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
