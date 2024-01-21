#include <cstdlib>

#include "java/util/ArrayList.txx"
#include "material/statistics.h"
#include "skin/Patch.h"
#include "skin/MeshSurface.h"

// Static counter that is increased each time a surface is created for making unique MeshSurface ids
static int globalNextSurfaceId = 0;

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
        case FACE_COLORS:
            break;
        case VERTEX_COLORS:
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
    PatchSet *faces,
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
    PatchSet *listStart = (PatchSet *)(surface->faces);
    if ( listStart ) {
        PatchSet *window;
        for ( window = listStart; window; window = window->next ) {
            Patch *patch = window->patch;
            surfaceConnectFace(surface, patch);
        }
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
This method will destroy the geometry and it's children geometries if any.

It is important that the patches be destroyed first and then the
vertices, because otherwise, the brep_data for the vertices would
still be used and thus not destroyed by the BREP library
*/
void
surfaceDestroy(MeshSurface *surface) {
    PatchSet *patchWindow = surface->faces;
    Patch *patch;
    while ( patchWindow != nullptr ) {
        patch = patchWindow->patch;
        patchWindow = patchWindow->next;
        patchDestroy(patch);
    }

    PatchSet *patchListNode;
    patchWindow = surface->faces;
    while ( patchWindow != nullptr ) {
        patchListNode = patchWindow->next;
        free(patchWindow);
        patchWindow = patchListNode;
    }

    if ( surface->vertices != nullptr ) {
        for ( int i = 0; i < surface->vertices->size(); i++) {
            vertexDestroy(surface->vertices->get(i));
        }
        delete surface->vertices;
    }

    Vector3DListNode *vector3DWindow = surface->positions;
    Vector3D *vectorPosition;
    while ( vector3DWindow != nullptr ) {
        vectorPosition = vector3DWindow->vector;
        vector3DWindow = vector3DWindow->next;
        vector3DDestroy(vectorPosition);
    }

    vector3DWindow = surface->positions;
    Vector3DListNode *vectorPositionNode;
    while ( vector3DWindow != nullptr ) {
        vectorPositionNode = vector3DWindow->next;
        free(vector3DWindow);
        vector3DWindow = vectorPositionNode;
    }

    Vector3DListNode *normalWindow = surface->normals;
    Vector3D *normal;
    while ( normalWindow != nullptr ) {
        normal = normalWindow->vector;
        normalWindow = normalWindow->next;
        vector3DDestroy(normal);
    }

    normalWindow = surface->normals;
    Vector3DListNode *normalNode;
    while ( normalWindow != nullptr ) {
        normalNode = normalWindow->next;
        free(normalWindow);
        normalWindow = normalNode;
    }

    Vector3DListNode *texCoordWindow = (surface->texCoords);
    Vector3D *texCoord;
    while ( texCoordWindow != nullptr ) {
        texCoord = texCoordWindow->vector;
        texCoordWindow = texCoordWindow->next;
        vector3DDestroy(texCoord);
    }

    texCoordWindow = surface->texCoords;
    Vector3DListNode *texCoordNode;
    while ( texCoordWindow != nullptr ) {
        texCoordNode = texCoordWindow->next;
        free(texCoordWindow);
        texCoordWindow = texCoordNode;
    }

    free(surface);
    GLOBAL_statistics_numberOfSurfaces--;
}

/**
This method will printRegularHierarchy the geometry to the file out
*/
void
surfacePrint(FILE *out, MeshSurface *surface) {
    fprintf(out, "MeshSurface id %d: material %s, patches ID: ",
            surface->id, surface->material->name);

    PatchSet *patchWindow = (surface->faces);
    Patch *patchElement;
    while (patchWindow) {
        patchElement = patchWindow->patch;
        patchWindow = patchWindow->next;
        patchPrintId(out, patchElement);
    }
    fprintf(out, "\n");

    PatchSet *patchWindow2 = (surface->faces);
    Patch *patchElement2;
    while (patchWindow2) {
        patchElement2 = patchWindow2->patch;
        patchWindow2 = patchWindow2->next;
        patchPrint(out, patchElement2);
    }
}

/**
This method will compute a bounding box for a geometry. The bounding box
is filled in bounding box and a pointer to the filled in bounding box
returned
*/
static float *
surfaceBounds(MeshSurface *surf, float *boundingbox) {
    return patchListBounds(surf->faces, boundingbox);
}

/**
Returns the list of patches making up a primitive geometry. This
method is not implemented for aggregate geometries
*/
static PatchSet *
surfacePatchList(MeshSurface *surf) {
    return surf->faces;
}

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
    (float *(*)(void *, float *)) surfaceBounds,
    (void (*)(void *)) surfaceDestroy,
    (void (*)(FILE *, void *)) surfacePrint,
    (GeometryListNode *(*)(void *)) nullptr,
    (PatchSet *(*)(void *)) surfacePatchList,
    (RayHit *(*)(void *, Ray *, float, float *, int, RayHit *)) surfaceDiscretizationIntersect,
    (HITLIST *(*)(HITLIST *, void *, Ray *, float, float, int)) surfaceAllDiscretizationIntersections,
    (void *(*)(void *)) nullptr
};
