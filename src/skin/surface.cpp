#include <cstdlib>

#include "common/Ray.h"
#include "material/statistics.h"
#include "skin/patch.h"
#include "skin/surface.h"
#include "skin/Vertex.h"
#include "skin/vertexlist.h"
#include "skin/Geometry.h"

/* a static counter that is increased each time a surface is created.
 * for making unique SURFACE ids. */
static int surfID = 0;

/**
Indicates on whether or not, and if so, which, colors are given when creating
a new surface
*/
enum COLORFLAGS colorFlags = NO_COLORS;

static void NormalizeVertexColor(VERTEX *vertex) {
    int nrpatches = 0;
    PATCHLIST *patches;
    for ( patches = vertex->patches; patches; patches = patches->next ) {
        nrpatches++;
    }
    if ( nrpatches > 0 ) {
        vertex->color.r /= (float) nrpatches;
        vertex->color.g /= (float) nrpatches;
        vertex->color.b /= (float) nrpatches;
    }
}

/* Fills in the SURFACE backpointer of the face belonging to the given
 * surface. */
void SurfaceConnectFace(SURFACE *surf, PATCH *face) {
    int i;
    COLOR rho;

    face->surface = surf;

    /* also fill in a nicer default color for the patch */
    switch ( colorFlags ) {
        case FACE_COLORS:
            break;
        case VERTEX_COLORS:
            /* average color of the vertices */
            RGBSET(face->color, 0, 0, 0);
            for ( i = 0; i < face->nrvertices; i++ ) {
                face->color.r += face->vertex[i]->color.r;
                face->color.g += face->vertex[i]->color.g;
                face->color.b += face->vertex[i]->color.b;
            }
            face->color.r /= (float) i;
            face->color.g /= (float) i;
            face->color.b /= (float) i;
            break;
        default: {
            rho = PatchAverageNormalAlbedo(face, BRDF_DIFFUSE_COMPONENT | BRDF_GLOSSY_COMPONENT);
            ColorToRGB(rho, &face->color);
        }
    }
}

/* This routine creates a SURFACE with given material, points, etc... */
SURFACE *SurfaceCreate(MATERIAL *material,
                       Vector3DListNode *points, Vector3DListNode *normals, Vector3DListNode *texCoords,
                       VERTEXLIST *vertices, PATCHLIST *faces,
                       enum COLORFLAGS flags) {
    SURFACE *surf;

    surf = (SURFACE *)malloc(sizeof(SURFACE));
    GLOBAL_statistics_numberOfSurfaces++;
    surf->id = surfID++;
    surf->material = material;
    surf->points = points;
    surf->normals = normals;
    surf->vertices = vertices;
    surf->texCoords = texCoords;
    surf->faces = faces;

    colorFlags = flags;

    /* if colroflags == VERTEX_COLORS< the vertices are assumed to contain
     * the sum of the colors as used in each patch sharing the vertex. */
    if ( colorFlags == VERTEX_COLORS ) VertexListIterate(surf->vertices, NormalizeVertexColor);

    /* fill in the SURFACE backpointer of the FACEs in the SURFACE. */
    ForAllPatches(face, surf->faces)
                {
                    SurfaceConnectFace(surf, face);
                }
    EndForAll;

    /* compute vertex colors */
    if ( colorFlags != VERTEX_COLORS ) VertexListIterate(surf->vertices, ComputeVertexColor);

    colorFlags = NO_COLORS;
    return surf;
}

/* This method will destroy the GEOMetry and it's children GEOMetries if 
 * any */
void SurfaceDestroy(SURFACE *surf) {
    /* It is important that the patches be destroyed first and then the
     * vertices, because otherwise, the brep_data for the vertices would
     * still be used and thus not destroyed by the BREP library. */
    PatchListIterate(surf->faces, PatchDestroy);
    PatchListDestroy(surf->faces);

    VertexListIterate(surf->vertices, VertexDestroy);
    VertexListDestroy(surf->vertices);

    VectorListIterate(surf->points, VectorDestroy);
    VectorListDestroy(surf->points);

    VectorListIterate(surf->normals, VectorDestroy);
    VectorListDestroy(surf->normals);

    VectorListIterate(surf->texCoords, VectorDestroy);
    VectorListDestroy(surf->texCoords);

    free(surf);
    GLOBAL_statistics_numberOfSurfaces--;
}

/* this method will print the GEOMetry to the file out */
void SurfacePrint(FILE *out, SURFACE *surface) {
    fprintf(out, "Surface id %d: material %s, patches ID: ",
            surface->id, surface->material->name);
    PatchListIterate1B(surface->faces, PatchPrintID, out);
    fprintf(out, "\n");

    PatchListIterate1B(surface->faces, PatchPrint, out);
}

/* this method will compute a bounding box for a GEOMetry. The bounding box
 * is filled in in boundingbox and a pointer to the filled in boundingbox 
 * returned. */
float *SurfaceBounds(SURFACE *surf, float *boundingbox) {
    return PatchListBounds(surf->faces, boundingbox);
}

/* returns the list of PATCHes making up a primitive GEOMetry. This
 * method is not implemented for aggregate GEOMetries. */
PATCHLIST *SurfacePatchlist(SURFACE *surf) {
    return surf->faces;
}

HITREC *
SurfaceDiscretisationIntersect(SURFACE *surf, Ray *ray, float mindist, float *maxdist, int hitflags, HITREC *hitstore) {
    return PatchListIntersect(surf->faces, ray, mindist, maxdist, hitflags, hitstore);
}

HITLIST *SurfaceAllDiscretisationIntersections(HITLIST *hits, SURFACE *surf, Ray *ray, float mindist, float maxdist,
                                               int hitflags) {
    return PatchListAllIntersections(hits, surf->faces, ray, mindist, maxdist, hitflags);
}

GEOM_METHODS surfaceMethods = {
        (float *(*)(void *, float *)) SurfaceBounds,
        (void (*)(void *)) SurfaceDestroy,
        (void (*)(FILE *, void *)) SurfacePrint,
        (GEOMLIST *(*)(void *)) nullptr,
        (PATCHLIST *(*)(void *)) SurfacePatchlist,
        (HITREC *(*)(void *, Ray *, float, float *, int, HITREC *)) SurfaceDiscretisationIntersect,
        (HITLIST *(*)(HITLIST *, void *, Ray *, float, float, int)) SurfaceAllDiscretisationIntersections,
        (void *(*)(void *)) nullptr
};
