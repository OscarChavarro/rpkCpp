#include "io/context/VertexRepository.h"

const Vector3Dd ZERO_VECTOR(0.0, 0.0, 0.0);

VertexRepository::VertexRepository():
    vertexLookUpTable(new LookUpTable(LookUpBehaviors::OWNING)),
    defaultVertexContext(ZERO_VECTOR, ZERO_VECTOR, 0, 1, nullptr),
    unNamedVertexContext(defaultVertexContext),
    currentVertex(&unNamedVertexContext)
{
}

VertexRepository::~VertexRepository() {
    if ( vertexLookUpTable != nullptr ) {
        delete vertexLookUpTable;
        vertexLookUpTable = nullptr;
    }
    currentVertex = nullptr;
}

void
VertexRepository::reset() {
    unNamedVertexContext = defaultVertexContext;
    currentVertex = &unNamedVertexContext;
    vertexLookUpTable->lookUpDone();
}
