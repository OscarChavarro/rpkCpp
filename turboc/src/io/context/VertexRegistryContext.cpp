#include "io/context/VertexRegistryContext.h"

const Vector3Dd VertexRegistryContext::ZERO_VECTOR(0.0, 0.0, 0.0);

VertexRegistryContext::VertexRegistryContext():
    vertexLookUpTable(new LookUpTable<char *>(OWNING)),
    defaultVertexContext(ZERO_VECTOR, ZERO_VECTOR, 0, 1, NULL),
    unNamedVertexContext(defaultVertexContext),
    currentVertex(&unNamedVertexContext)
{
}

VertexRegistryContext::~VertexRegistryContext() {
    if ( vertexLookUpTable != NULL ) {
        delete vertexLookUpTable;
        vertexLookUpTable = NULL;
    }
    currentVertex = NULL;
}

void
VertexRegistryContext::reset() {
    unNamedVertexContext = defaultVertexContext;
    currentVertex = &unNamedVertexContext;
    vertexLookUpTable->lookUpDone();
}
