#include "material/statistics.h"
#include "skin/Vertex.h"

static unsigned int vertex_compare_flags = VCMP_LOCATION | VCMP_NORMAL | VCMP_TEXCOORD;

/**
Create a vertex with given coordinates, normal vector and list of patches
sharing the vertex. Several vertices can share the same coordinates
and normal vector. Several patches can share the same vertex
*/
VERTEX *
VertexCreate(Vector3D *point, Vector3D *normal, Vector3D *texCoord, PATCHLIST *patches) {
    VERTEX *v = (VERTEX *)malloc(sizeof(VERTEX));

    v->id = GLOBAL_statistics_numberOfVertices++;
    v->point = point;
    v->normal = normal;
    v->texCoord = texCoord;
    v->patches = patches;
    RGBSET(v->color, 0., 0., 0.);
    v->radiance_data = (void *)nullptr;
    v->back = (VERTEX *)nullptr;

    return v;
}

/**
Destroys the vertex. Does not destroy the coordinate vector and
normal vector, neither the patches sharing the vertex
*/
void
VertexDestroy(VERTEX *vertex) {
    PatchListDestroy(vertex->patches);
    free(vertex);
    GLOBAL_statistics_numberOfVertices--;
}

/**
Prints the vertex data to the file 'out'
*/
void
VertexPrint(FILE *out, VERTEX *vertex) {
    fprintf(out, "ID %d: (", vertex->id);
    VectorPrint(out, *(vertex->point));
    fprintf(out, ")");
    if ( vertex->normal ) {
        fprintf(out, "/(");
        VectorPrint(out, *(vertex->normal));
        fprintf(out, ")");
    } else {
        fprintf(out, "/(no normal)");
    }
    if ( vertex->texCoord ) {
        fprintf(out, "/(");
        VectorPrint(out, *(vertex->texCoord));
        fprintf(out, ")");
    } else {
        fprintf(out, "/(no texCoord)");
    }
    fprintf(out, " color = (");
    RGBPrint(out, vertex->color);
    fprintf(out, "),");

    fprintf(out, "patches: ");
    ForAllPatches(P, vertex->patches)
                {
                    PatchPrintID(out, P);
                }
    EndForAll

    fprintf(out, "\n");
}

/**
Averages the color of each patch sharing the vertex and assign the 
resulting color to the vertex
*/
void
ComputeVertexColor(VERTEX *vertex) {
    PATCHLIST *patches;
    int nrpatches;

    RGBSET(vertex->color, 0., 0., 0.);
    nrpatches = 0;
    for ( patches = vertex->patches; patches; patches = patches->next ) {
        vertex->color.r += patches->patch->color.r;
        vertex->color.g += patches->patch->color.g;
        vertex->color.b += patches->patch->color.b;
        nrpatches++;
    }
    if ( nrpatches > 0 ) {
        vertex->color.r /= (float) nrpatches;
        vertex->color.g /= (float) nrpatches;
        vertex->color.b /= (float) nrpatches;
    }
}

/**
Computes a vertex color for the vertices of the patch
*/
void
PatchComputeVertexColors(PATCH *patch) {
    int i;

    for ( i = 0; i < patch->nrvertices; i++ ) {
        ComputeVertexColor(patch->vertex[i]);
    }
}

unsigned
VertexSetCompareFlags(unsigned flags) {
    unsigned oldflags = vertex_compare_flags;
    vertex_compare_flags = flags;
    return oldflags;
}

/**
This routine only compares the location of the two vertices
*/
int
VertexCompareLocation(VERTEX *v1, VERTEX *v2) {
    /* two entities intersect if their tolerance region intersects, i.o.w. if
     * the distance between them is smaller than the sum of the two entity's
     * tolerances */
    float tolerance = VECTORTOLERANCE(*v1->point) + VECTORTOLERANCE(*v2->point);
    return vectorCompareByDimensions(v1->point, v2->point, tolerance);
}

/**
This routine only compares the normal of the two vertices
*/
int
VertexCompareNormal(VERTEX *v1, VERTEX *v2) {
    int code = vectorCompareByDimensions(v1->normal, v2->normal, EPSILON);

    if ( code == XYZ_EQUAL && !(vertex_compare_flags & VCMP_NO_NORMAL_IS_EQUAL_NORMAL)) {
        if ( v1->normal->x == 0. && v1->normal->y == 0. && v1->normal->z == 0. ) {
            return 0;
        }
    }
    return code; // no valid normal => not the same vertex
}

/**
This routine only compares the texture coordinates of the two vertices
*/
int
VertexCompareTexCoord(VERTEX *v1, VERTEX *v2) {
    if ( !v1->texCoord ) {
        if ( !v2->texCoord ) {
            return XYZ_EQUAL;
        } else {
            return 0;
        } // no coordinate == smaller than any coordinate
    } else {
        if ( !v2->texCoord ) {
            return X_GREATER + Y_GREATER + Z_GREATER;
        } else {
            return vectorCompareByDimensions(v1->texCoord, v2->texCoord, EPSILON);
        }
    }
}

/**
Compares two vertices and returns a code useful for ordering vertices
into an octree. Returns 8 if the two vertices are to be considered
the same vertices and 0-7, the index of an octree branch to be expored
further, if not
*/
int
VertexCompare(VERTEX *v1, VERTEX *v2) {
    int code = XYZ_EQUAL;

    // First compare the coordinates
    if ( vertex_compare_flags & VCMP_LOCATION ) {
        code = VertexCompareLocation(v1, v2);
        if ( code != XYZ_EQUAL ) {
            return code;
        }
    }

    // Same coordinates, test the normals
    if ( vertex_compare_flags & VCMP_NORMAL ) {
        code = VertexCompareNormal(v1, v2);
        if ( code != XYZ_EQUAL ) {
            return code;
        }
    }

    // Same coordinates and same normal, test texture coordinates
    if ( vertex_compare_flags & VCMP_TEXCOORD ) {
        code = VertexCompareTexCoord(v1, v2);
        if ( code != XYZ_EQUAL ) {
            return code;
        }
    }

    return XYZ_EQUAL;
}
