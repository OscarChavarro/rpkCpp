#include "vsdk/toolkit/java/util/ArrayList.txx"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "Vertex.h"

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
    id = Statistics::instance().reader.numberOfVertices++;
    point = inPoint;
    normal = inNormal;
    textureCoordinates = inTextureCoordinates;
    patches = inPatches;
    color.set(0.0, 0.0, 0.0);
    radianceData = nullptr;
    back = static_cast<Vertex *>(nullptr);
}

/**
Destroys the vertex. Does not destroy the coordinate vector and
normal vector, neither the patches sharing the vertex
*/
Vertex::~Vertex() {
    Statistics::instance().reader.numberOfVertices--;
    delete patches;
}

/**
Averages the color of each patch sharing the vertex and assign the 
resulting color to the vertex
*/
void
Vertex::computeColor() {
    long numberOfPatches;

    color.set(0.0, 0.0, 0.0);
    numberOfPatches = 0;

    if ( patches != nullptr ) {
        for ( int i = 0; i < patches->size(); i++) {
            const Patch *patch = patches->get(i);
            color.setR(color.getR() + patch->getColor().getR());
            color.setG(color.getG() + patch->getColor().getG());
            color.setB(color.getB() + patch->getColor().getB());
        }
        numberOfPatches = patches->size();
    }

    if ( numberOfPatches > 0 ) {
        color.setR(color.getR() / static_cast<double>(numberOfPatches));
        color.setG(color.getG() / static_cast<double>(numberOfPatches));
        color.setB(color.getB() / static_cast<double>(numberOfPatches));
    }
}

unsigned
Vertex::setCompareFlags(unsigned flags) {
    unsigned oldFlags = currentComparisonFlags;
    currentComparisonFlags = flags;
    return oldFlags;
}
