#include "java/util/ArrayList.txx"
#include "material/statistics.h"
#include "skin/Vertex.h"

static unsigned int vertex_compare_flags = VERTEX_COMPARE_LOCATION | VERTEX_COMPARE_NORMAL | VERTEX_COMPARE_TEXTURE_COORDINATE;

Vertex::Vertex(): id(), point(), normal(),
texCoord(), color(), radiance_data(), back(), patches(),
tmp()
{
}

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
    setRGB(v->color, 0.0f, 0.0f, 0.0f);
    v->radiance_data = nullptr;
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
    vector3DPrint(out, *(vertex->point));
    fprintf(out, ")");
    if ( vertex->normal ) {
        fprintf(out, "/(");
        vector3DPrint(out, *(vertex->normal));
        fprintf(out, ")");
    } else {
        fprintf(out, "/(no normal)");
    }
    if ( vertex->texCoord ) {
        fprintf(out, "/(");
        vector3DPrint(out, *(vertex->texCoord));
        fprintf(out, ")");
    } else {
        fprintf(out, "/(no texCoord)");
    }
    fprintf(out, " color = (");
    printRGB(out, vertex->color);
    fprintf(out, "),");

    fprintf(out, "patches: ");
    for ( int i = 0; vertex->patches != nullptr && i < vertex->patches->size(); i++ ) {
        vertex->patches->get(i)->patchPrintId(out);
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

    setRGB(vertex->color, 0.0f, 0.0f, 0.0f);
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
    unsigned oldFlags = vertex_compare_flags;
    vertex_compare_flags = flags;
    return oldFlags;
}
