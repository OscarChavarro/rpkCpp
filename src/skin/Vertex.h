#ifndef __VERTEX__
#define __VERTEX__

#include "java/util/ArrayList.h"
#include "common/linealAlgebra/Vector3D.h"
#include "material/color.h"
#include "skin/PatchSet.h"

class Patch;

class VERTEX {
  public:
    int id; // id number for debugging etc.
    Vector3D *point; // pointer to the coordinates of the vertex
    Vector3D *normal; // pointer to the normal vector at the vertex
    Vector3D *texCoord; // texture coordinates
    RGB color; // color for the vertex when rendering with Gouraud interpolation
    void *radiance_data; // data for the vertex maintained by the current radiance method
    VERTEX *back; // vertex at the same position, but with reversed normal, for back faces
    java::ArrayList<PATCH *> *patches; // list of patches sharing the vertex
    int tmp; /* some temporary storage for vertices, used e.g. for saving VRML. Do not
                assume the contents of this storage remain unchanged after leaving
		control to the user. */
};

extern VERTEX *VertexCreate(Vector3D *point, Vector3D *normal, Vector3D *texCoord, java::ArrayList<PATCH *> *patches);
extern void VertexDestroy(VERTEX *vertex);
extern void VertexPrint(FILE *out, VERTEX *vertex);
extern void ComputeVertexColor(VERTEX *vertex);
extern void PatchComputeVertexColors(PATCH *patch);

/**
Vertex comparison

Vertices have a coordinate, normal and texture coordinate.
The following flags determine what is taken into account
when comparing vertices:
- VCMP_LOCATION: compare location
- VCMP_NORMAL: compare normal
- VCMP_TEXCOORD: compare texture coordinates
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

#define VCMP_LOCATION 0x01
#define VCMP_NORMAL 0x02
#define VCMP_TEXCOORD 0x04
#define VCMP_NO_NORMAL_IS_EQUAL_NORMAL 0x80

extern unsigned VertexSetCompareFlags(unsigned flags);
extern int VertexCompare(VERTEX *v1, VERTEX *v2);
extern int VertexCompareLocation(VERTEX *v1, VERTEX *v2);

#include "skin/Patch.h"

#endif
