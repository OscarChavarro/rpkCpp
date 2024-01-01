/**
Surfaces are basically a list of PATCHes representing a simple object with given Material.
*/
#ifndef __SURFACE__
#define __SURFACE__

#include <cstdio>

#include "java/util/ArrayList.h"
#include "material/material.h"
#include "skin/vectorlist.h"
#include "skin/vertexlist.h"
#include "skin/Vertex.h"
#include "skin/geomlist.h"

class GEOM_METHODS;
class GEOM;
class PATCHLIST;

class SURFACE {
  public:
    /* an id which can be used for debugging. */
    int id;

    /* MATERIAL the surface is made of */
    MATERIAL *material;

    /* a list of points at the vertices of the patches of the surface */
    Vector3DListNode *points;

    /* a list of normals at the vertices of the patches. */
    Vector3DListNode *normals;

    /* a list of texture coordinates of the vertices of the patches */
    Vector3DListNode *texCoords;

    /* the vertices of the patches. Each vertex contains a pointer to the vertex
     * coordinates and normal vector at the vertex, which are in the SURFACEs 'points'
     * and 'normals' list. Different vertices can share the same coordinates and/or
     * normals. */
    VERTEXLIST *vertices;

    /* the PATCHes making up the SURFACE. Each PATCH contains pointers to three
     * or four VERTEXes in the 'vertices' list of the SURFACE. Different PATCHes
     * can share the same VERTEX. Each VERTEX also contains a list of pointers to
     * the PATCHes that share the VERTEX. This can be used for e.g. Gouraud shading
     * if a color is assigned to each VERTEX. Each PATCH also contains a backpointer to
     * to SURFACE to which it belongs. */
    PATCHLIST *faces;
};

enum COLORFLAGS {
    NO_COLORS, VERTEX_COLORS, FACE_COLORS
};

/* This routine creates a SURFACE with given material, points, etc... */
extern SURFACE *SurfaceCreate(MATERIAL *material,
                              Vector3DListNode *points, Vector3DListNode *normals, Vector3DListNode *texCoords,
                              VERTEXLIST *vertices, PATCHLIST *faces,
                              enum COLORFLAGS flags);

/* A set of pointers to functions to operate on a SURFACE, as declared in geom.h. */
extern GEOM_METHODS surfaceMethods;
#define SurfaceMethods()    (&surfaceMethods)

/* tests whether a Geometry is a SURFACE. */
#define GeomIsSurface(geom)    (geom->methods == &surfaceMethods)

/* "cast" a geom to a surface */
#define GeomGetSurface(geom)    (GeomIsSurface(geom) ? (SURFACE*)(geom->obj) : (SURFACE*)nullptr)

#include "skin/Geometry.h"

#endif
