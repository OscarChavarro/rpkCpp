#include "java/util/ArrayList.txx"
#include "material/statistics.h"
#include "skin/Vertex.h"

static unsigned int vertex_compare_flags = VERTEX_COMPARE_LOCATION | VERTEX_COMPARE_NORMAL | VERTEX_COMPARE_TEXTURE_COORDINATE;

Vertex::~Vertex() {
    if ( patches ) {
        // TODO: Check if should delete all content
        for ( int i = 0; patches != nullptr && i < patches->size(); i++) {
            //delete patches->get(i);
        }
        delete patches;
    }
}

/**
Create a vertex with given coordinates, normal vector and list of patches
sharing the vertex. Several vertices can share the same coordinates
and normal vector. Several patches can share the same vertex
*/
Vertex *
vertexCreate(Vector3D *point, Vector3D *normal, Vector3D *texCoord, java::ArrayList<Patch *> *patches) {
    Vertex *v = new Vertex();

    v->id = GLOBAL_statistics_numberOfVertices++;
    v->point = point;
    v->normal = normal;
    v->texCoord = texCoord;
    v->patches = patches;
    RGBSET(v->color, 0., 0., 0.);
    v->radiance_data = (void *)nullptr;
    v->back = (Vertex *)nullptr;

    return v;
}

/**
Destroys the vertex. Does not destroy the coordinate vector and
normal vector, neither the patches sharing the vertex
*/
void
vertexDestroy(Vertex *vertex) {
    delete vertex;
    GLOBAL_statistics_numberOfVertices--;
}

/**
Prints the vertex data to the file 'out'
*/
void
vertexPrint(FILE *out, Vertex *vertex) {
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
    for ( int i = 0; vertex->patches != nullptr && i < vertex->patches->size(); i++ ) {
        patchPrintId(out, vertex->patches->get(i));
    }

    fprintf(out, "\n");
}

/**
Averages the color of each patch sharing the vertex and assign the 
resulting color to the vertex
*/
void
computeVertexColor(Vertex *vertex) {
    long numberOfPatches;

    RGBSET(vertex->color, 0.0f, 0.0f, 0.0f);
    numberOfPatches = 0;

    if ( vertex->patches != nullptr ) {
        for ( int i = 0; i < vertex->patches->size(); i++) {
            Patch *p = vertex->patches->get(i);
            vertex->color.r += p->color.r;
            vertex->color.g += p->color.g;
            vertex->color.b += p->color.b;
        }
        numberOfPatches = vertex->patches->size();
    }

    if ( numberOfPatches > 0 ) {
        vertex->color.r /= (float) numberOfPatches;
        vertex->color.g /= (float) numberOfPatches;
        vertex->color.b /= (float) numberOfPatches;
    }
}

/**
Computes a vertex color for the vertices of the patch
*/
void
patchComputeVertexColors(Patch *patch) {
    int i;

    for ( i = 0; i < patch->numberOfVertices; i++ ) {
        computeVertexColor(patch->vertex[i]);
    }
}

unsigned
vertexSetCompareFlags(unsigned flags) {
    unsigned oldflags = vertex_compare_flags;
    vertex_compare_flags = flags;
    return oldflags;
}

/**
This routine only compares the location of the two vertices
*/
int
vertexCompareLocation(Vertex *v1, Vertex *v2) {
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
vertexCompareNormal(Vertex *v1, Vertex *v2) {
    int code = vectorCompareByDimensions(v1->normal, v2->normal, EPSILON);

    if ( code == XYZ_EQUAL && !(vertex_compare_flags & VERTEX_COMPARE_NO_NORMAL_IS_EQUAL_NORMAL)) {
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
vertexCompareTexCoord(Vertex *v1, Vertex *v2) {
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
vertexCompare(Vertex *v1, Vertex *v2) {
    int code = XYZ_EQUAL;

    // First compare the coordinates
    if ( vertex_compare_flags & VERTEX_COMPARE_LOCATION ) {
        code = vertexCompareLocation(v1, v2);
        if ( code != XYZ_EQUAL ) {
            return code;
        }
    }

    // Same coordinates, test the normals
    if ( vertex_compare_flags & VERTEX_COMPARE_NORMAL ) {
        code = vertexCompareNormal(v1, v2);
        if ( code != XYZ_EQUAL ) {
            return code;
        }
    }

    // Same coordinates and same normal, test texture coordinates
    if ( vertex_compare_flags & VERTEX_COMPARE_TEXTURE_COORDINATE ) {
        code = vertexCompareTexCoord(v1, v2);
        if ( code != XYZ_EQUAL ) {
            return code;
        }
    }

    return XYZ_EQUAL;
}
