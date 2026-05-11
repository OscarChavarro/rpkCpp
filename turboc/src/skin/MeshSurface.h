/**
Surfaces are basically a list of patches representing a simple object with given material
*/

#ifndef __SURFACE__
#define __SURFACE__

#include "java/util/ArrayList.h"
#include "material/Material.h"
#include "skin/Geometry.h"
#include "material/MaterialColorFlags.h"
#include "environment/geometry/elements/Vertex.h"

class MeshSurface: public Geometry{ private:
    static int nextSurfaceId;

    static void normalizeVertexColor(Vertex *vertex);

  public:
    int meshId;
    char *objectName;

    static MaterialColorFlags colorFlags;

    /**
    The vertices of the patches. Each vertex contains a pointer to the vertex
    coordinates and normal vector at the vertex, which are in the MeshSurface 'positions'
    and 'normals' list. Different vertices can share the same coordinates and/or
    normals
    */
    ArrayList<Vertex *> *vertices;

    // A list of positions at the vertices of the patches of the surface
    ArrayList<Vector3D *> *positions;

    // A list of normals at the vertices of the patches
    ArrayList<Vector3D *> *normals;

    /**
    The patches making up the MeshSurface. Each patch contains pointers to three
    or four vertices in the 'vertices' list of the MeshSurface. Different patches
    can share the same Vertex. Each vertex also contains a list of pointers to
    the patches that share the vertex. This can be used for e.g. Gouraud shading
    if a color is assigned to each vertex. Each patch also contains a back pointer to
    to MeshSurface to which it belongs
    */
    ArrayList<Patch *> *faces;

    Material *material;

    MeshSurface( char *inObjectName, Material *inMaterial, ArrayList<Vector3D *> *inPoints, ArrayList<Vector3D *> *inNormals, const ArrayList<Vector3D *> * /*texCoords*/, ArrayList<Vertex *> *inVertices, ArrayList<Patch *> *inFaces, MaterialColorFlags inFlags);
    ~MeshSurface();

    RayHit *
    discretizationIntersect( Ray *ray, float minimumDistance, float *maximumDistance, int hitFlags, RayHit *hitStore) const;
};

#endif
