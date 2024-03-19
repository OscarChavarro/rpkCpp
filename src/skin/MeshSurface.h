/**
Surfaces are basically a list of patches representing a simple object with given material
*/

#ifndef __SURFACE__
#define __SURFACE__

#include <cstdio>

#include "java/util/ArrayList.h"
#include "material/Material.h"
#include "skin/Geometry.h"

class Vertex;

enum MaterialColorFlags {
    NO_COLORS,
    VERTEX_COLORS,
    FACE_COLORS
};

class MeshSurface : public Geometry {
  public:
    int id;

    /**
    The vertices of the patches. Each vertex contains a pointer to the vertex
    coordinates and normal vector at the vertex, which are in the MeshSurface 'positions'
    and 'normals' list. Different vertices can share the same coordinates and/or
    normals
    */
    java::ArrayList<Vertex *> *vertices;

    // A list of positions at the vertices of the patches of the surface
    java::ArrayList<Vector3D *> *positions;

    // A list of normals at the vertices of the patches
    java::ArrayList<Vector3D *> *normals;

    /**
    The patches making up the MeshSurface. Each patch contains pointers to three
    or four vertices in the 'vertices' list of the MeshSurface. Different patches
    can share the same Vertex. Each vertex also contains a list of pointers to
    the patches that share the vertex. This can be used for e.g. Gouraud shading
    if a color is assigned to each vertex. Each patch also contains a back pointer to
    to MeshSurface to which it belongs
    */
    java::ArrayList<Patch *> *faces;

    Material *material;

    MeshSurface();
    MeshSurface(
        Material *material,
        java::ArrayList<Vector3D *> *points,
        java::ArrayList<Vector3D *> *normals,
        java::ArrayList<Vector3D *> * /*texCoords*/,
        java::ArrayList<Vertex *> *vertices,
        java::ArrayList<Patch *> *faces,
        enum MaterialColorFlags flags);

    RayHit *
    discretizationIntersect(
        Ray *ray,
        float minimumDistance,
        float *maximumDistance,
        int hitFlags,
        RayHit *hitStore);
};

extern BoundingBox *
surfaceBounds(MeshSurface *surf, BoundingBox *boundingBox);

#endif
