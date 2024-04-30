#include "java/util/ArrayList.txx"
#include "material/statistics.h"
#include "skin/Vertex.h"

unsigned int Vertex::currentComparisonFlags = VERTEX_COMPARE_LOCATION | VERTEX_COMPARE_NORMAL | VERTEX_COMPARE_TEXTURE_COORDINATE;

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
    color.set(0.0f, 0.0f, 0.0f);
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
Vertex::computeColor() {
    long numberOfPatches;

    color.set(0.0f, 0.0f, 0.0f);
    numberOfPatches = 0;

    if ( patches != nullptr ) {
        for ( int i = 0; i < patches->size(); i++) {
            const Patch *patch = patches->get(i);
            color.r += patch->color.r;
            color.g += patch->color.g;
            color.b += patch->color.b;
        }
        numberOfPatches = patches->size();
    }

    if ( numberOfPatches > 0 ) {
        color.r /= (float) numberOfPatches;
        color.g /= (float) numberOfPatches;
        color.b /= (float) numberOfPatches;
    }
}

unsigned
Vertex::setCompareFlags(unsigned flags) {
    unsigned oldFlags = currentComparisonFlags;
    currentComparisonFlags = flags;
    return oldFlags;
}
