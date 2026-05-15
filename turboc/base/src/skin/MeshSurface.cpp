#include "java/util/ArrayList.txx"
#include "common/statistics/Statistics.h"
#include "skin/MeshSurface.h"

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
    ArrayList<Vector3D *> *inPoints,
    ArrayList<Vector3D *> *inNormals,
    const ArrayList<Vector3D *> * /*texCoords*/,
    ArrayList<Vertex *> *inVertices,
    ArrayList<Patch *> *inFaces,
    MaterialColorFlags inFlags)
{
    Statistics::instance().reader.numberOfSurfaces++;

    id = nextGeometryId;
    nextGeometryId++;
    objectName = inObjectName;
    meshId = nextSurfaceId++;
    className = SURFACE_MESH;
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
        for ( int i = 0; vertices != NULL && i < vertices->size(); i++ ) {
            MeshSurface::normalizeVertexColor(vertices->get(i));
        }
    }

    // Compute vertex colors
    if ( colorFlags != VERTEX_COLORS ) {
        for ( int i = 0; vertices != NULL && i < vertices->size(); i++ ) {
            vertices->get(i)->computeColor();
        }
    }

    colorFlags = NO_COLORS;

    patchListBounds(faces, &boundingBox);

    // Enlarge bounding box a tiny bit for more conservative bounding box culling
    boundingBox.enlargeTinyBit();
    bounded = true;
    shaftCullGeometry = false;
    radianceData = NULL;
    itemCount = 0;
    omit = false;
}

MeshSurface::~MeshSurface() {
    if ( objectName ) {
        delete[] objectName;
    }

    if ( positions != NULL) {
        for ( int i = 0; i < positions->size(); i++ ) {
            delete positions->get(i);
        }
        delete positions;
    }

    if ( normals != NULL ) {
        for ( int i = 0; i < normals->size(); i++ ) {
            delete normals->get(i);
        }
        delete normals;
    }

    if ( vertices != NULL ) {
        for ( int i = 0; i < vertices->size(); i++ ) {
            delete vertices->get(i);
        }
        delete vertices;
    }

    if ( faces != NULL ) {
        for ( int i = 0; i < faces->size(); i++ ) {
            delete faces->get(i);
        }
        delete faces;
    }
}

void
MeshSurface::normalizeVertexColor(Vertex *vertex) {
    long numberOfPatches = 0;

    if ( vertex->patches != NULL ) {
        numberOfPatches = vertex->patches->size();
    }

    if ( numberOfPatches > 0 ) {
        const float inv = 1.0f / ((float)(numberOfPatches));
        vertex->color = ColorRgb(
            vertex->color.getR() * inv,
            vertex->color.getG() * inv,
            vertex->color.getB() * inv);
    }
}

/**
DiscretizationIntersect returns NULL is the ray doesn't hit the discretization
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
