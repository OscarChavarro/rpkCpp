/**
Surfaces are basically a list of PATCHes representing a simple object with given Material.
*/

#ifndef __SURFACE__
#define __SURFACE__

#include <cstdio>

#include "java/util/ArrayList.h"
#include "material/Material.h"
#include "skin/vectorlist.h"
#include "skin/geomlist.h"

class GEOM_METHODS;
class Geometry;
class PatchSet;
class Vertex;

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
    Vector3DListNode *positions;

    // A list of normals at the vertices of the patches
    Vector3DListNode *normals;

    // A list of texture coordinates of the vertices of the patches
    Vector3DListNode *texCoords;

    /**
    The patches making up the MeshSurface. Each patch contains pointers to three
    or four vertices in the 'vertices' list of the MeshSurface. Different patches
    can share the same Vertex. Each vertex also contains a list of pointers to
    the patches that share the vertex. This can be used for e.g. Gouraud shading
    if a color is assigned to each vertex. Each patch also contains a back pointer to
    to MeshSurface to which it belongs
    */
    PatchSet *faces;

    Material *material;
};

enum MaterialColorFlags {
    NO_COLORS,
    VERTEX_COLORS,
    FACE_COLORS
};

extern MeshSurface *
surfaceCreate(
        Material *material,
        Vector3DListNode *points,
        Vector3DListNode *normals,
        Vector3DListNode *texCoords,
        java::ArrayList<Vertex *> *vertices,
        PatchSet *faces,
        MaterialColorFlags flags);

// A set of pointers to functions to operate on a MeshSurface
extern GEOM_METHODS GLOBAL_skin_surfaceGeometryMethods;

inline bool
geomIsSurface(Geometry *geom) {
    return geom->methods == &GLOBAL_skin_surfaceGeometryMethods;
}

inline MeshSurface*
geomGetSurface(Geometry *geom) {
    return geomIsSurface(geom) ? geom->surfaceData : nullptr;
}

#include "skin/Geometry.h"

#endif
