#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkVertex.h"
#include "vsdk/toolkit/scene/FaceColorTable.h"

unsigned int RpkVertex::currentComparisonFlags = VERTEX_COMPARE_LOCATION | VERTEX_COMPARE_NORMAL | VERTEX_COMPARE_TEXTURE_COORDINATE;

/**
Create a vertex with given coordinates, inNormal vector and list of inPatches
sharing the vertex. Several vertices can share the same coordinates
and inNormal vector. Several inPatches can share the same vertex
*/
RpkVertex::RpkVertex(
    Vector3Df *inPoint,
    Vector3Df *inNormal,
    Vector3Df *inTextureCoordinates,
    java::ArrayList<Patch *> *inPatches):
    color(),
    tmp()
{
    id = Statistics::instance().reader.numberOfVertices++;
    point = inPoint;
    normal = inNormal;
    textureCoordinates = inTextureCoordinates;
    patches = inPatches;
    color = ColorRgbMutable(0.0, 0.0, 0.0);
    radianceData = nullptr;
    back = static_cast<RpkVertex *>(nullptr);
}

/**
Destroys the vertex. Does not destroy the coordinate vector and
normal vector, neither the patches sharing the vertex
*/
RpkVertex::~RpkVertex() {
    Statistics::instance().reader.numberOfVertices--;
    delete patches;
}

/**
Averages the color of each patch sharing the vertex and assign the 
resulting color to the vertex
*/
void
RpkVertex::computeColor(const FaceColorTable *faceColors) {
    long numberOfPatches;

    color = ColorRgbMutable(0.0, 0.0, 0.0);
    numberOfPatches = 0;

    if ( patches != nullptr ) {
        double r = 0.0;
        double g = 0.0;
        double b = 0.0;
        for ( int i = 0; i < patches->size(); i++) {
            // Before the scene's face table exists (loading) every face is black
            if ( faceColors != nullptr ) {
                const ColorRgbMutable &patchColor = faceColors->get(patches->get(i)->getFaceIndex());
                r += patchColor.getR();
                g += patchColor.getG();
                b += patchColor.getB();
            }
        }
        color = ColorRgbMutable(r, g, b);
        numberOfPatches = patches->size();
    }

    if ( numberOfPatches > 0 ) {
        const double invPatchCount = 1.0 / static_cast<double>(numberOfPatches);
        color = ColorRgbMutable(color.getR() * invPatchCount, color.getG() * invPatchCount, color.getB() * invPatchCount);
    }
}

unsigned
RpkVertex::setCompareFlags(unsigned flags) {
    unsigned oldFlags = currentComparisonFlags;
    currentComparisonFlags = flags;
    return oldFlags;
}
