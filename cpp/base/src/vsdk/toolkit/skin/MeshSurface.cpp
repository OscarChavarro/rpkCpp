#include "vsdk/toolkit/java/util/ArrayList.txx"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/skin/MeshSurface.h"

int MeshSurface::nextSurfaceId = 0;

/**
Indicates on whether or not, and if so, which, colors are given when creating
a new surface
*/
MaterialColorFlags MeshSurface::colorFlags = NO_COLORS;

/**
This routine creates a MeshSurface with given inMaterial, positions
*/
MeshSurface::MeshSurface(
    char *inObjectName,
    Material *inMaterial,
    java::ArrayList<Vector3D *> *inPoints,
    java::ArrayList<Vector3D *> *inNormals,
    const java::ArrayList<Vector3D *> * /*texCoords*/,
    java::ArrayList<Vertex *> *inVertices,
    java::ArrayList<Patch *> *inFaces,
    MaterialColorFlags inFlags)
{
    Statistics::instance().reader.numberOfSurfaces++;

    id = nextGeometryId;
    nextGeometryId++;
    objectName = inObjectName;
    meshId = nextSurfaceId++;
    className = GeometryClassId::SURFACE_MESH;
    isDuplicate = false;

    material = inMaterial;
    positions = inPoints;
    normals = inNormals;
    vertices = inVertices;
    faces = inFaces;

    colorFlags = inFlags;

    // If colorFlags == VERTEX_COLORS< the inVertices are assumed to contain
    // the sum of the colors as used in each patch sharing the vertex
    if ( colorFlags == VERTEX_COLORS ) {
        for ( int i = 0; vertices != nullptr && i < vertices->size(); i++ ) {
            MeshSurface::normalizeVertexColor(vertices->get(i));
        }
    }

    // Compute vertex colors
    if ( colorFlags != VERTEX_COLORS ) {
        for ( int i = 0; vertices != nullptr && i < vertices->size(); i++ ) {
            vertices->get(i)->computeColor();
        }
    }

    colorFlags = NO_COLORS;

    patchListBounds(faces, &boundingBox);

    // Enlarge bounding box a tiny bit for more conservative bounding box culling
    boundingBox.enlargeTinyBit();
    bounded = true;
    shaftCullGeometry = false;
    radianceData = nullptr;
    itemCount = 0;
    omit = false;
}

MeshSurface::~MeshSurface() {
    if ( objectName ) {
        delete[] objectName;
    }

    if ( positions != nullptr) {
        for ( int i = 0; i < positions->size(); i++ ) {
            delete positions->get(i);
        }
        delete positions;
    }

    if ( normals != nullptr ) {
        for ( int i = 0; i < normals->size(); i++ ) {
            delete normals->get(i);
        }
        delete normals;
    }

    if ( vertices != nullptr ) {
        for ( int i = 0; i < vertices->size(); i++ ) {
            delete vertices->get(i);
        }
        delete vertices;
    }

    if ( faces != nullptr ) {
        for ( int i = 0; i < faces->size(); i++ ) {
            delete faces->get(i);
        }
        delete faces;
    }
}

void
MeshSurface::normalizeVertexColor(Vertex *vertex) {
    long numberOfPatches = 0;

    if ( vertex->patches != nullptr ) {
        numberOfPatches = vertex->patches->size();
    }

    if ( numberOfPatches > 0 ) {
        const double invPatchCount = 1.0 / static_cast<double>(numberOfPatches);
        vertex->color = ColorRgb(
            vertex->color.getR() * invPatchCount,
            vertex->color.getG() * invPatchCount,
            vertex->color.getB() * invPatchCount);
    }
}

/**
DiscretizationIntersect returns nullptr is the ray doesn't hit the discretization
of the object. If the ray hits the object, a hit record is returned containing
information about the intersection point. See geometry.h for more explanation
*/
RayHit *
MeshSurface::discretizationIntersect(
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore) const
{
    return patchListIntersect(faces, ray, minimumDistance, maximumDistance, hitFlags, hitStore);
}
