#ifndef __VERTEX__
#define __VERTEX__

#include "java/util/ArrayList.h"
#include "common/linealAlgebra/Vector3D.h"
#include "common/color.h"

class Element;
class Patch;

class Vertex {
  public:
    int id; // id number for debugging etc.
    Vector3D *point; // pointer to the coordinates of the vertex
    Vector3D *normal; // pointer to the normal vector at the vertex
    Vector3D *texCoord; // texture coordinates
    RGB color; // color for the vertex when rendering with Gouraud interpolation
    java::ArrayList<Element *> *radianceData; // Data for the vertex maintained by the current radiance method
    Vertex *back; // vertex at the same position, but with reversed normal, for back faces
    java::ArrayList<Patch *> *patches; // list of patches sharing the vertex
    int tmp; /* some temporary storage for vertices, used e.g. for saving VRML. Do not
                assume the contents of this storage remain unchanged after leaving
		control to the user. */

    Vertex();
    virtual ~Vertex();
};

extern Vertex *vertexCreate(Vector3D *point, Vector3D *normal, Vector3D *texCoord, java::ArrayList<Patch *> *patches);
extern void vertexDestroy(Vertex *vertex);
extern void computeVertexColor(Vertex *vertex);
extern void patchComputeVertexColors(Patch *patch);

/**
Vertex comparison

Vertices have a coordinate, normal and texture coordinate.
The following flags determine what is taken into account
when comparing vertices:
- VERTEX_COMPARE_LOCATION: compare location
- VERTEX_COMPARE_NORMAL: compare normal
- VERTEX_COMPARE_TEXTURE_COORDINATE: compare texture coordinates
The flags are set using SetVertexCompareFlags().

The comparison order is as follows:
First the location is compared (if so requested).
If location is equal, compare the normals (if requested)
If location and normal is equal, compare texture coordinates.

The vertex comparison routines return
XYZ_EQUAL: is the vertices are equal
a code from 0 to 7 if the vertices are not equal. This code can be used
to sort vertices in an octree. The code is a combination of the flags
X_GREATER, Y_GREATER and Z_GREATER and is the same as for vectorCompareByDimensions in
Vector3D.h
*/

#define VERTEX_COMPARE_LOCATION 0x01
#define VERTEX_COMPARE_NORMAL 0x02
#define VERTEX_COMPARE_TEXTURE_COORDINATE 0x04

extern unsigned vertexSetCompareFlags(unsigned flags);

#include "skin/Patch.h"
#include "skin/Element.h"

#endif
