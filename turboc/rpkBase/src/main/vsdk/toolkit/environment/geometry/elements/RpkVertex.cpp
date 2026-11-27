#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkVertex.h"

unsigned int RpkVertex::currentComparisonFlags = VERTEX_COMPARE_LOCATION | VERTEX_COMPARE_NORMAL | VRTX_CMPR_TXTR_CRDNT;

/**
Create a vertex with given coordinates, inNormal vector and list of inPatches
sharing the vertex. Several vertices can share the same coordinates
and inNormal vector. Several inPatches can share the same vertex
*/
RpkVertex::RpkVertex(
    Vector3Dd *inPoint,
    Vector3Dd *inNormal,
    Vector3Dd *inTextureCoordinates,
    ArrayList<Patch *> *inPatches):
    color(),
    tmp()
{
    id = Statistics::instance().reader.numberOfVertices++;
    point = inPoint;
    normal = inNormal;
    textureCoordinates = inTextureCoordinates;
    patches = inPatches;
    color = ColorRgb(0.0, 0.0, 0.0);
    radianceData = NULL;
    back = ((RpkVertex *)(NULL));
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
RpkVertex::computeColor() {
    long numberOfPatches;

    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
    numberOfPatches = 0;

    if ( patches != NULL ) {
        for ( int i = 0; i < patches->size(); i++) {
            const Patch *patch = patches->get(i);
            r += patch->color.getR();
            g += patch->color.getG();
            b += patch->color.getB();
        }
        numberOfPatches = patches->size();
    }

    if ( numberOfPatches > 0 ) {
        const double inv = 1.0 / ((double)(numberOfPatches));
        color = ColorRgb(r * inv, g * inv, b * inv);
    } else {
        color = ColorRgb(0.0, 0.0, 0.0);
    }
}

unsigned
RpkVertex::setCompareFlags(unsigned flags) {
    unsigned oldFlags = currentComparisonFlags;
    currentComparisonFlags = flags;
    return oldFlags;
}
