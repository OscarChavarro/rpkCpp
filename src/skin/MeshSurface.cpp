#include <cstdlib>

#include "java/util/ArrayList.txx"
#include "material/statistics.h"
#include "skin/Patch.h"
#include "skin/MeshSurface.h"

// Static counter that is increased each time a surface is created for making unique MeshSurface ids
static int globalNextSurfaceId = 0;

MeshSurface::MeshSurface(): id(), vertices(), positions(), normals(), texCoords(), faces(), material() {
}

/**
Indicates on whether or not, and if so, which, colors are given when creating
a new surface
*/
enum MaterialColorFlags globalColorFlags = NO_COLORS;

static void
normalizeVertexColor(Vertex *vertex) {
    long numberOfPatches = 0;

    if ( vertex->patches != nullptr ) {
        numberOfPatches = vertex->patches->size();
    }

    if ( numberOfPatches > 0 ) {
        vertex->color.r /= (float) numberOfPatches;
        vertex->color.g /= (float) numberOfPatches;
        vertex->color.b /= (float) numberOfPatches;
    }
}

/**
Fills in the MeshSurface back pointer of the face belonging to the given surface
*/
void
surfaceConnectFace(MeshSurface *surf, Patch *face) {
    int i;
    COLOR rho;

    face->surface = surf;

    // Also fill in a nicer default color for the patch
    switch ( globalColorFlags ) {
        case MaterialColorFlags::FACE_COLORS:
            break;
        case MaterialColorFlags::VERTEX_COLORS:
            // Average color of the vertices
            setRGB(face->color, 0, 0, 0);
            for ( i = 0; i < face->numberOfVertices; i++ ) {
                face->color.r += face->vertex[i]->color.r;
                face->color.g += face->vertex[i]->color.g;
                face->color.b += face->vertex[i]->color.b;
            }
            face->color.r /= (float) i;
            face->color.g /= (float) i;
            face->color.b /= (float) i;
            break;
        default: {
            rho = patchAverageNormalAlbedo(face, BRDF_DIFFUSE_COMPONENT | BRDF_GLOSSY_COMPONENT);
            convertColorToRGB(rho, &face->color);
        }
    }
}

/**
This routine creates a MeshSurface with given material, positions
*/
MeshSurface *
surfaceCreate(
    Material *material,
    Vector3DListNode *points,
    Vector3DListNode *normals,
    Vector3DListNode *texCoords,
    java::ArrayList<Vertex *> *vertices,
    java::ArrayList<Patch *> *faces,
    enum MaterialColorFlags flags)
{
    MeshSurface *surface;

    surface = (MeshSurface *)malloc(sizeof(MeshSurface));
    GLOBAL_statistics_numberOfSurfaces++;
    surface->id = globalNextSurfaceId++;
    surface->material = material;
    surface->positions = points;
    surface->normals = normals;
    surface->vertices = vertices;
    surface->texCoords = texCoords;
    surface->faces = faces;

    globalColorFlags = flags;

    // If globalColorFlags == VERTEX_COLORS< the vertices are assumed to contain
    // the sum of the colors as used in each patch sharing the vertex
    if ( globalColorFlags == VERTEX_COLORS ) {
        for ( int i = 0; surface->vertices != nullptr && i < surface->vertices->size(); i++ ) {
            normalizeVertexColor(surface->vertices->get(i));
        }
    }

    // Fill in the MeshSurface back pointer of the FACEs in the MeshSurface
    for ( int i = 0; surface->faces != nullptr && i < surface->faces->size(); i++ ) {
        surfaceConnectFace(surface, surface->faces->get(i));
    }

    // Compute vertex colors
    if ( globalColorFlags != VERTEX_COLORS ) {
        for ( int i = 0; surface->vertices != nullptr && i < surface->vertices->size(); i++ ) {
            computeVertexColor(surface->vertices->get(i));
        }
    }

    globalColorFlags = NO_COLORS;
    return surface;
}

/**
This method will compute a bounding box for a geometry. The bounding box
is filled in bounding box and a pointer to the filled in bounding box
returned
*/
float *
surfaceBounds(MeshSurface *surf, float *boundingBox) {
    return patchListBounds(surf->faces, boundingBox);
}

/**
DiscretizationIntersect returns nullptr is the ray doesn't hit the discretization
of the object. If the ray hits the object, a hit record is returned containing
information about the intersection point. See geometry.h for more explanation
*/
RayHit *
surfaceDiscretizationIntersect(
    MeshSurface *surf,
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore)
{
    return patchListIntersect(surf->faces, ray, minimumDistance, maximumDistance, hitFlags, hitStore);
}

HITLIST *
surfaceAllDiscretizationIntersections(
    HITLIST *hits,
    MeshSurface *surf,
    Ray *ray,
    float minimumDistance,
    float maximumDistance,
    int hitFlags)
{
    return patchListAllIntersections(hits, surf->faces, ray, minimumDistance, maximumDistance, hitFlags);
}

GEOM_METHODS GLOBAL_skin_surfaceGeometryMethods = {
    nullptr,
    (HITLIST *(*)(HITLIST *, void *, Ray *, float, float, int)) surfaceAllDiscretizationIntersections
};
