#include "java/util/ArrayList.txx"
#include "material/statistics.h"
#include "skin/Patch.h"
#include "skin/MeshSurface.h"

// Static counter that is increased each time a surface is created for making unique MeshSurface ids
static int globalNextSurfaceId = 0;

MeshSurface::MeshSurface(): id(), vertices(), positions(), normals(), faces(), material() {
}

MeshSurface::~MeshSurface() {
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
        vertex->color.r /= (float)numberOfPatches;
        vertex->color.g /= (float)numberOfPatches;
        vertex->color.b /= (float)numberOfPatches;
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
            rho = face->averageNormalAlbedo(BRDF_DIFFUSE_COMPONENT | BRDF_GLOSSY_COMPONENT);
            convertColorToRGB(rho, &face->color);
        }
    }
}

/**
This routine creates a MeshSurface with given material, positions
*/
MeshSurface::MeshSurface(
    Material *material,
    java::ArrayList<Vector3D *> *points,
    java::ArrayList<Vector3D *> *normals,
    java::ArrayList<Vector3D *> * /*texCoords*/,
    java::ArrayList<Vertex *> *vertices,
    java::ArrayList<Patch *> *faces,
    enum MaterialColorFlags flags)
{
    GLOBAL_statistics.numberOfSurfaces++;

    this->id = globalNextSurfaceId++;
    this->compoundData = nullptr;
    this->patchSetData = nullptr;
    this->className = GeometryClassId::SURFACE_MESH;
    this->isDuplicate = false;

    this->material = material;
    this->positions = points;
    this->normals = normals;
    this->vertices = vertices;
    this->faces = faces;
    this->className = GeometryClassId::SURFACE_MESH;

    globalColorFlags = flags;

    // If globalColorFlags == VERTEX_COLORS< the vertices are assumed to contain
    // the sum of the colors as used in each patch sharing the vertex
    if ( globalColorFlags == VERTEX_COLORS ) {
        for ( int i = 0; this->vertices != nullptr && i < this->vertices->size(); i++ ) {
            normalizeVertexColor(this->vertices->get(i));
        }
    }

    // Fill in the MeshSurface back pointer of the FACEs in the MeshSurface
    for ( int i = 0; this->faces != nullptr && i < this->faces->size(); i++ ) {
        surfaceConnectFace(this, this->faces->get(i));
    }

    // Compute vertex colors
    if ( globalColorFlags != VERTEX_COLORS ) {
        for ( int i = 0; this->vertices != nullptr && i < this->vertices->size(); i++ ) {
            computeVertexColor(this->vertices->get(i));
        }
    }

    globalColorFlags = NO_COLORS;

    patchListBounds(this->faces, &boundingBox);

    // Enlarge bounding box a tiny bit for more conservative bounding box culling
    this->boundingBox.enlargeTinyBit();
    this->bounded = true;
    this->shaftCullGeometry = false;
    this->radianceData = nullptr;
    this->itemCount = 0;
    this->omit = false;
    this->displayListId = -1;
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
