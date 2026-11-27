/**
Surfaces are basically a list of patches representing a simple object with given material
*/

#ifndef __MESH_SURFACE__
#define __MESH_SURFACE__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/material/RpkMaterial.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"
#include "vsdk/toolkit/material/MaterialColorFlags.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkVertex.h"

class MeshSurface: public RpkGeometry{ private:
    static int nextSurfaceId;

    static void normalizeVertexColor(RpkVertex *vertex);

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
    ArrayList<RpkVertex *> *vertices;

    // A list of positions at the vertices of the patches of the surface
    ArrayList<Vector3Dd *> *positions;

    // A list of normals at the vertices of the patches
    ArrayList<Vector3Dd *> *normals;

    /**
    The patches making up the MeshSurface. Each patch contains pointers to three
    or four vertices in the 'vertices' list of the MeshSurface. Different patches
    can share the same RpkVertex. Each vertex also contains a list of pointers to
    the patches that share the vertex. This can be used for e.g. Gouraud shading
    if a color is assigned to each vertex. Each patch also contains a back pointer to
    to MeshSurface to which it belongs
    */
    ArrayList<Patch *> *faces;

    RpkMaterial *material;

    MeshSurface( char *inObjectName, RpkMaterial *inMaterial, ArrayList<Vector3Dd *> *inPoints, ArrayList<Vector3Dd *> *inNormals, const ArrayList<Vector3Dd *> * /*texCoords*/, ArrayList<RpkVertex *> *inVertices, ArrayList<Patch *> *inFaces, MaterialColorFlags inFlags);
    ~MeshSurface();

    RpkRayHit *
    discretizationIntersect( Ray *ray, double minimumDistance, double *maximumDistance, int hitFlags, RpkRayHit *hitStore, const RayExclusionSet &exclusions) const;
};

#endif
