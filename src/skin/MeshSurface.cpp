#include <cstdlib>

#include "common/Ray.h"
#include "material/statistics.h"
#include "skin/Patch.h"
#include "skin/MeshSurface.h"
#include "skin/Vertex.h"
#include "skin/vertexlist.h"
#include "skin/Geometry.h"

/* a static counter that is increased each time a surface is created.
 * for making unique MeshSurface ids. */
static int surfID = 0;

/**
Indicates on whether or not, and if so, which, colors are given when creating
a new surface
*/
enum MaterialColorFlags colorFlags = NO_COLORS;

static void NormalizeVertexColor(VERTEX *vertex) {
    int nrpatches = 0;
    PatchSet *patches;
    for ( patches = vertex->patches; patches; patches = patches->next ) {
        nrpatches++;
    }
    if ( nrpatches > 0 ) {
        vertex->color.r /= (float) nrpatches;
        vertex->color.g /= (float) nrpatches;
        vertex->color.b /= (float) nrpatches;
    }
}

/* Fills in the MeshSurface backpointer of the face belonging to the given
 * surface. */
void SurfaceConnectFace(MeshSurface *surf, PATCH *face) {
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
            convertColorToRGB(rho, &face->color);
        }
    }
}

/* This routine creates a MeshSurface with given material, positions, etc... */
MeshSurface *surfaceCreate(Material *material,
                           Vector3DListNode *points, Vector3DListNode *normals, Vector3DListNode *texCoords,
                           VERTEXLIST *vertices, PatchSet *faces,
                           enum MaterialColorFlags flags) {
    MeshSurface *surf;

    surf = (MeshSurface *)malloc(sizeof(MeshSurface));
    GLOBAL_statistics_numberOfSurfaces++;
    surf->id = surfID++;
    surf->material = material;
    surf->positions = points;
    surf->normals = normals;
    surf->vertices = vertices;
    surf->texCoords = texCoords;
    surf->faces = faces;

    colorFlags = flags;

    /* if colroflags == VERTEX_COLORS< the vertices are assumed to contain
     * the sum of the colors as used in each patch sharing the vertex. */
    if ( colorFlags == VERTEX_COLORS ) VertexListIterate(surf->vertices, NormalizeVertexColor);

    /* fill in the MeshSurface backpointer of the FACEs in the MeshSurface. */
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
void SurfaceDestroy(MeshSurface *surf) {
    /* It is important that the patches be destroyed first and then the
     * vertices, because otherwise, the brep_data for the vertices would
     * still be used and thus not destroyed by the BREP library. */
    PatchListIterate(surf->faces, PatchDestroy);
    PatchListDestroy(surf->faces);

    VertexListIterate(surf->vertices, VertexDestroy);
    VertexListDestroy(surf->vertices);

    VectorListIterate(surf->positions, VectorDestroy);
    VectorListDestroy(surf->positions);

    VectorListIterate(surf->normals, VectorDestroy);
    VectorListDestroy(surf->normals);

    VectorListIterate(surf->texCoords, VectorDestroy);
    VectorListDestroy(surf->texCoords);

    free(surf);
    GLOBAL_statistics_numberOfSurfaces--;
}

/* this method will print the GEOMetry to the file out */
void SurfacePrint(FILE *out, MeshSurface *surface) {
    fprintf(out, "Surface id %d: material %s, patches ID: ",
            surface->id, surface->material->name);
    PatchListIterate1B(surface->faces, PatchPrintID, out);
    fprintf(out, "\n");

    PatchListIterate1B(surface->faces, PatchPrint, out);
}

/* this method will compute a bounding box for a GEOMetry. The bounding box
 * is filled in in boundingbox and a pointer to the filled in boundingbox 
 * returned. */
float *SurfaceBounds(MeshSurface *surf, float *boundingbox) {
    return PatchListBounds(surf->faces, boundingbox);
}

/* returns the list of PATCHes making up a primitive GEOMetry. This
 * method is not implemented for aggregate GEOMetries. */
PatchSet *SurfacePatchlist(MeshSurface *surf) {
    return surf->faces;
}

HITREC *
SurfaceDiscretisationIntersect(MeshSurface *surf, Ray *ray, float mindist, float *maxdist, int hitflags, HITREC *hitstore) {
    return PatchListIntersect(surf->faces, ray, mindist, maxdist, hitflags, hitstore);
}

HITLIST *SurfaceAllDiscretisationIntersections(HITLIST *hits, MeshSurface *surf, Ray *ray, float mindist, float maxdist,
                                               int hitflags) {
    return PatchListAllIntersections(hits, surf->faces, ray, mindist, maxdist, hitflags);
}

bool
GeomIsSurface(Geometry *geom) {
    return geom->methods == &GLOBAL_skin_surfaceGeometryMethods;
}

MeshSurface*
GeomGetSurface(Geometry *geom) {
    return GeomIsSurface(geom) ? geom->surfaceData : nullptr;
}

GEOM_METHODS GLOBAL_skin_surfaceGeometryMethods = {
        (float *(*)(void *, float *)) SurfaceBounds,
        (void (*)(void *)) SurfaceDestroy,
        (void (*)(FILE *, void *)) SurfacePrint,
        (GeometryListNode *(*)(void *)) nullptr,
        (PatchSet *(*)(void *)) SurfacePatchlist,
        (HITREC *(*)(void *, Ray *, float, float *, int, HITREC *)) SurfaceDiscretisationIntersect,
        (HITLIST *(*)(HITLIST *, void *, Ray *, float, float, int)) SurfaceAllDiscretisationIntersections,
        (void *(*)(void *)) nullptr
};
