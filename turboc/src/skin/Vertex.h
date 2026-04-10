#ifndef __VERTEX__
#define __VERTEX__

#include "java/util/ArrayList.h"
#include "common/linealAlgebra/Vector3D.h"
#include "common/ColorRgb.h"
#include "skin/VertexCompareFlags.h"

class Element;
class Patch;

class Vertex {
  private:
    static unsigned int currentComparisonFlags;

  public:
    int id;
    Vector3D *point;
    Vector3D *normal;
    Vector3D *textureCoordinates;
    ColorRgb color; // Used when rendering with Gouraud interpolation
    ArrayList<Element *> *radianceData; // Data for the vertex maintained by the current radiance method
    Vertex *back; // Vertex at the same position, but with reversed normal, for back faces
    ArrayList<Patch *> *patches; // List of references to patches sharing the vertex
    int tmp; // Temporary (transient) storage for vertices used for saving VRML. Do not assume the contents of
             // this storage remain unchanged after leaving control to the user

    explicit Vertex(
        Vector3D *inPoint,
        Vector3D *inNormal,
        Vector3D *inTextureCoordinates,
        ArrayList<Patch *> *inPatches);
    virtual ~Vertex();

    void computeColor();
    static unsigned setCompareFlags(unsigned flags);
};

/**
Vertex comparison

Vertices have a coordinate, normal and texture coordinate.
The following flags determine what is taken into account
when comparing vertices:
- VERTEX_COMPARE_LOCATION: compare location
- VERTEX_COMPARE_NORMAL: compare normal
- VRTX_CMPR_TXTR_CRDNT: compare texture coordinates
The flags are set using setCompareFlags().

The comparison order is as follows:
- First the location is compared (if so requested)
- If location is equal, compare the normals (if requested)
- If location and normal is equal, compare texture coordinates

The vertex comparison routines return
- XYZ_EQUAL_MASK: is the vertices are equal
- A code from 0 to 7 if the vertices are not equal. This code can be used
  to sort vertices in an octree. The code is a combination of the flags
  X_GREATER_MASK, Y_GREATER_MASK and Z_GREATER_MASK and is the same as for Vertex::setCompareFlags in
  Vector3D
*/

#include "skin/Patch.h"
#include "skin/Element.h"

#endif
