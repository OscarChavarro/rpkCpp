#ifndef __VERTEX__
#define __VERTEX__

#include "java/util/ArrayList.h"
#include "common/linealAlgebra/Vector3D.h"
#include "common/ColorRgb.h"

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
    java::ArrayList<Element *> *radianceData; // Data for the vertex maintained by the current radiance method
    Vertex *back; // Vertex at the same position, but with reversed normal, for back faces
    java::ArrayList<Patch *> *patches; // List of references to patches sharing the vertex
    int tmp; // Temporary (transient) storage for vertices used for saving VRML. Do not assume the contents of
             // this storage remain unchanged after leaving control to the user

    explicit Vertex(
        Vector3D *inPoint,
        Vector3D *inNormal,
        Vector3D *inTextureCoordinates,
        java::ArrayList<Patch *> *inPatches);
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
- VERTEX_COMPARE_TEXTURE_COORDINATE: compare texture coordinates
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

#define VERTEX_COMPARE_LOCATION 0x01
#define VERTEX_COMPARE_NORMAL 0x02
#define VERTEX_COMPARE_TEXTURE_COORDINATE 0x04

#include "skin/Patch.h"
#include "skin/Element.h"

#endif
