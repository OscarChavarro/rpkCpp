#ifndef __SCREEN_POLYGON__
#define __SCREEN_POLYGON__

#include "java/lang/System.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/convexPolygonClipper/PolygonVertex.h"

/**
Immutable convex polygon with homogeneous vertices, as clipped by
ConvexWindowClipper. The mask is an interpolation mask whose kth bit
indicates whether the kth attribute of a PolygonVertex is relevant. For
example, if the valid attributes are sx, sy, and sz, then set
mask = ScreenPolygon::attributeBit(PolygonVertex::SX) |
       ScreenPolygon::attributeBit(PolygonVertex::SY) |
       ScreenPolygon::attributeBit(PolygonVertex::SZ);
*/
class ScreenPolygon {
  public:
    // Clipping an n-gon against k planes can give an (n + k)-gon
    enum { MAXIMUM_SIDES_PER_POLYGON = 10 };

  private:
    int numberOfVertices;
    unsigned long mask; // Interpolation mask for vertex elems
    PolygonVertex vertices[MAXIMUM_SIDES_PER_POLYGON];

  public:
    /** The empty polygon */
    ScreenPolygon(): numberOfVertices(0), mask(0UL) {}

    /** Copies the first inNumberOfVertices vertices of inVertices */
    ScreenPolygon(int inNumberOfVertices, unsigned long inMask, const PolygonVertex *inVertices):
        numberOfVertices(inNumberOfVertices), mask(inMask)
    {
        int i;
        if ( inNumberOfVertices < 0 || inNumberOfVertices > MAXIMUM_SIDES_PER_POLYGON ) {
            System::err.printf("ScreenPolygon: invalid number of vertices: %d (max=%d)\n",
                inNumberOfVertices, (int)MAXIMUM_SIDES_PER_POLYGON);
            System::exit(1);
        }
        for ( i = 0; i < inNumberOfVertices; i++ ) {
            vertices[i] = inVertices[i];
        }
    }

    static unsigned long
    attributeBit(int attributeIndex) {
        return 1UL << attributeIndex;
    }

    int getNumberOfVertices() const { return numberOfVertices; }
    unsigned long getMask() const { return mask; }
    const PolygonVertex &getVertex(int index) const { return vertices[index]; }
};

#endif
