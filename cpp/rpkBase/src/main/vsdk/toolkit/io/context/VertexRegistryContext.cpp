#include "vsdk/toolkit/io/context/VertexRegistryContext.h"

const Vector3Dd VertexRegistryContext::ZERO_VECTOR(0.0, 0.0, 0.0);

VertexRegistryContext::VertexRegistryContext():
    vertexLookUpTable(new LookUpTable<char *>(LookUpBehaviors::OWNING)),
    defaultVertexContext(ZERO_VECTOR, ZERO_VECTOR, 0, 1, nullptr),
    unNamedVertexContext(defaultVertexContext),
    currentVertex(&unNamedVertexContext)
{
}

VertexRegistryContext::~VertexRegistryContext() {
    if ( vertexLookUpTable != nullptr ) {
        delete vertexLookUpTable;
        vertexLookUpTable = nullptr;
    }
    currentVertex = nullptr;
}

void
VertexRegistryContext::reset() {
    unNamedVertexContext = defaultVertexContext;
    currentVertex = &unNamedVertexContext;
    vertexLookUpTable->lookUpDone();
}
