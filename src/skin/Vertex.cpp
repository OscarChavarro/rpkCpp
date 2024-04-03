#include "java/util/ArrayList.txx"
#include "material/statistics.h"
#include "skin/Vertex.h"

static unsigned int globalCurrentVertexComparisonFlags = VERTEX_COMPARE_LOCATION | VERTEX_COMPARE_NORMAL | VERTEX_COMPARE_TEXTURE_COORDINATE;

/**
Create a vertex with given coordinates, inNormal vector and list of inPatches
sharing the vertex. Several vertices can share the same coordinates
and inNormal vector. Several inPatches can share the same vertex
*/
Vertex::Vertex(
    Vector3D *inPoint,
    Vector3D *inNormal,
    Vector3D *inTextureCoordinates,
    java::ArrayList<Patch *> *inPatches):
    color(),
    tmp()
{
    id = GLOBAL_statistics.numberOfVertices++;
    point = inPoint;
    normal = inNormal;
    textureCoordinates = inTextureCoordinates;
    patches = inPatches;
    setRGB(color, 0.0f, 0.0f, 0.0f);
    radianceData = nullptr;
    back = (Vertex *)nullptr;
}

/**
Destroys the vertex. Does not destroy the coordinate vector and
normal vector, neither the patches sharing the vertex
*/
Vertex::~Vertex() {
    GLOBAL_statistics.numberOfVertices--;
    delete patches;
}

/**
Averages the color of each patch sharing the vertex and assign the 
resulting color to the vertex
*/
void
computeVertexColor(Vertex *vertex) {
    long numberOfPatches;

    setRGB(vertex->color, 0.0f, 0.0f, 0.0f);
    numberOfPatches = 0;

    if ( vertex->patches != nullptr ) {
        for ( int i = 0; i < vertex->patches->size(); i++) {
            Patch *p = vertex->patches->get(i);
            vertex->color.r += p->color.r;
            vertex->color.g += p->color.g;
            vertex->color.b += p->color.b;
        }
        numberOfPatches = vertex->patches->size();
    }

    if ( numberOfPatches > 0 ) {
        vertex->color.r /= (float) numberOfPatches;
        vertex->color.g /= (float) numberOfPatches;
        vertex->color.b /= (float) numberOfPatches;
    }
}

/**
Computes a vertex color for the vertices of the patch
*/
void
patchComputeVertexColors(Patch *patch) {
    for ( int i = 0; i < patch->numberOfVertices; i++ ) {
        computeVertexColor(patch->vertex[i]);
    }
}

unsigned
vertexSetCompareFlags(unsigned flags) {
    unsigned oldFlags = globalCurrentVertexComparisonFlags;
    globalCurrentVertexComparisonFlags = flags;
    return oldFlags;
}
