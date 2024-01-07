#include <cstdlib>

#include "java/util/ArrayList.txx"
#include "common/Ray.h"
#include "material/statistics.h"
#include "skin/Patch.h"
#include "skin/MeshSurface.h"
#include "skin/Vertex.h"
#include "skin/Geometry.h"

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
surfaceConnectFace(MeshSurface *surf, PATCH *face) {
    int i;
    COLOR rho;

    face->surface = surf;

    /* also fill in a nicer default color for the patch */
    switch ( globalColorFlags ) {
        case FACE_COLORS:
            break;
        case VERTEX_COLORS:
            /* average color of the vertices */
            RGBSET(face->color, 0, 0, 0);
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
    MeshSurface *surf;

    surf = (MeshSurface *)malloc(sizeof(MeshSurface));
    GLOBAL_statistics_numberOfSurfaces++;
    surf->id = globalNextSurfaceId++;
    surf->material = material;
    surf->positions = points;
    surf->normals = normals;
    surf->vertices = vertices;
    surf->texCoords = texCoords;
    surf->faces = faces;

    globalColorFlags = flags;

    // If globalColorFlags == VERTEX_COLORS< the vertices are assumed to contain
    // the sum of the colors as used in each patch sharing the vertex
    if ( globalColorFlags == VERTEX_COLORS ) {
        for ( int i = 0; surf->vertices != nullptr && i < surf->vertices->size(); i++ ) {
            normalizeVertexColor(surf->vertices->get(i));
        }
    }

    /* fill in the MeshSurface backpointer of the FACEs in the MeshSurface. */
    ForAllPatches(face, surf->faces)
                {
                    surfaceConnectFace(surf, face);
                }
    EndForAll;

    // Compute vertex colors
    if ( globalColorFlags != VERTEX_COLORS ) {
        for ( int i = 0; surf->vertices != nullptr && i < surf->vertices->size(); i++ ) {
            computeVertexColor(surf->vertices->get(i));
        }
    }

    globalColorFlags = NO_COLORS;
    return surf;
}

/**
This method will destroy the geometry and it's children geometries if any.

It is important that the patches be destroyed first and then the
vertices, because otherwise, the brep_data for the vertices would
still be used and thus not destroyed by the BREP library
*/
void
surfaceDestroy(MeshSurface *surf) {
    /* It is important that the patches be destroyed first and then the
     * vertices, because otherwise, the brep_data for the vertices would
     * still be used and thus not destroyed by the BREP library. */
    PatchListIterate(surf->faces, patchDestroy);
    PatchListDestroy(surf->faces);

    if ( surf->vertices != nullptr ) {
        for ( int i = 0; i < surf->vertices->size(); i++) {
            vertexDestroy(surf->vertices->get(i));
        }
        delete surf->vertices;
    }

    VectorListIterate(surf->positions, VectorDestroy);
    VectorListDestroy(surf->positions);

    VectorListIterate(surf->normals, VectorDestroy);
    VectorListDestroy(surf->normals);

    VectorListIterate(surf->texCoords, VectorDestroy);
    VectorListDestroy(surf->texCoords);

    free(surf);
    GLOBAL_statistics_numberOfSurfaces--;
}

/**
This method will print the geometry to the file out
*/
void
surfacePrint(FILE *out, MeshSurface *surface) {
    fprintf(out, "Surface id %d: material %s, patches ID: ",
            surface->id, surface->material->name);
    PatchListIterate1B(surface->faces, patchPrintId, out);
    fprintf(out, "\n");

    PatchListIterate1B(surface->faces, patchPrint, out);
}

/**
This method will compute a bounding box for a geometry. The bounding box
is filled in bounding box and a pointer to the filled in bounding box
returned
*/
static float *
surfaceBounds(MeshSurface *surf, float *boundingbox) {
    return PatchListBounds(surf->faces, boundingbox);
}

/**
Returns the list of patches making up a primitive geometry. This
method is not implemented for aggregate geometries
*/
static PatchSet *
surfacePatchList(MeshSurface *surf) {
    return surf->faces;
}

HITREC *
surfaceDiscretizationIntersect(MeshSurface *surf, Ray *ray, float mindist, float *maxdist, int hitflags, HITREC *hitstore) {
    return PatchListIntersect(surf->faces, ray, mindist, maxdist, hitflags, hitstore);
}

HITLIST *
surfaceAllDiscretizationIntersections(HITLIST *hits, MeshSurface *surf, Ray *ray, float mindist, float maxdist,
                                      int hitflags) {
    return PatchListAllIntersections(hits, surf->faces, ray, mindist, maxdist, hitflags);
}

GEOM_METHODS GLOBAL_skin_surfaceGeometryMethods = {
    (float *(*)(void *, float *)) surfaceBounds,
    (void (*)(void *)) surfaceDestroy,
    (void (*)(FILE *, void *)) surfacePrint,
    (GeometryListNode *(*)(void *)) nullptr,
    (PatchSet *(*)(void *)) surfacePatchList,
    (HITREC *(*)(void *, Ray *, float, float *, int, HITREC *)) surfaceDiscretizationIntersect,
    (HITLIST *(*)(HITLIST *, void *, Ray *, float, float, int)) surfaceAllDiscretizationIntersections,
    (void *(*)(void *)) nullptr
};
