//= References:                                                             =
//= [SUTH1974b] Sutherland, Ivan E.; Hodgman, Gary W. "Reentrant Polygon  =
//=          Clipping", Communications of the ACM 17(1), 32-42, 1974.     =
//=          DOI: 10.1145/360767.360802.                                  =
//= [HECK1990] Heckbert, Paul S. "Generic Convex Polygon Scan Conversion   =
//=          and Clipping", Graphics Gems, ed. Andrew S. Glassner,         =
//=          Academic Press, 1990, section II.5, pp. 84-86.                 =

/**
Clipping against a convex window of [SUTH1974b]: the window is an intersection
of homogeneous half-spaces, and the single-plane routine is the one of the
homogeneous 3-D convex polygon clipper of [HECK1990] (Paul Heckbert 1985,
Dec 1989), generalized from the box planes to arbitrary planes.

Note that original algorithm is available at
https://github.com/erich666/GraphicsGems/tree/master/gems/PolyScan
*/

#include "java/lang/System.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/convexPolygonClipper/ConvexWindowClipper.h"

/**
Signed plane measure of the vertex for the half-space ([SUTH1974b], p. 36):
negative or zero inside, positive outside.
*/
inline double
ConvexWindowClipper::signedMeasure(const PolygonVertex &vertex, const HomogeneousHalfSpace &halfSpace) {
    return halfSpace.getXCoefficient() * vertex.getSx() + halfSpace.getYCoefficient() * vertex.getSy() +
           halfSpace.getZCoefficient() * vertex.getSz() + halfSpace.getWCoefficient() * vertex.getSw();
}

/**
Stops the program when q, of ScreenPolygon::MAXIMUM_SIDES_PER_POLYGON
vertices, has no room for one more. Clipping a convex polygon against a plane
adds at most one vertex, but an input that leaves no room for it, or one that
is not convex, would otherwise write past the end of q.
*/
void
ConvexWindowClipper::checkRoomForVertex(int numberOfVertices) {
    if ( numberOfVertices >= ScreenPolygon::MAXIMUM_SIDES_PER_POLYGON ) {
        java::System::err.printf("ConvexWindowClipper: too many vertices: %d (max=%d)\n",
            numberOfVertices + 1, ScreenPolygon::MAXIMUM_SIDES_PER_POLYGON);
        java::System::exit(1);
    }
}

/**
Clip convex polygon p against a half-space, returning the portion satisfying
a * sx + b * sy + c * sz + d * sw <= 0 (see HomogeneousHalfSpace). The
attributes selected by the mask of p are interpolated at the edge crossings;
the others are zero in the vertices created there.
*/
ScreenPolygon
ConvexWindowClipper::clipToHalfSpace(const ScreenPolygon &p, const HomogeneousHalfSpace &halfSpace) {
    const int n = p.getNumberOfVertices();
    const unsigned long mask = p.getMask();
    PolygonVertex q[ScreenPolygon::MAXIMUM_SIDES_PER_POLYGON];
    int qn = 0;

    if ( n == 0 ) {
        // A previous clipping stage may have emptied the polygon; without
        // this guard, vertex n - 1 below is undefined behavior
        return ScreenPolygon(0, mask, q);
    }

    // Close the polygon by processing the last-to-first edge first
    // ([SUTH1974b], pp. 34-35), without repeating the first input vertex.
    int previousVertexIndex = n - 1;
    double tu = ConvexWindowClipper::signedMeasure(p.getVertex(previousVertexIndex), halfSpace);
    for ( int currentVertexIndex = 0; currentVertexIndex < n; currentVertexIndex++ ) {
        // On old polygon (p), u is previous vertex, v is current vertex
        // tv is negative if vertex v is in
        const PolygonVertex &u = p.getVertex(previousVertexIndex);
        const PolygonVertex &v = p.getVertex(currentVertexIndex);
        const double tv = ConvexWindowClipper::signedMeasure(v, halfSpace);
        // The four inside/outside edge cases are those of [SUTH1974b],
        // p. 34, Fig. 2: emit an intersection on a crossing, followed by
        // the terminal vertex if it is inside. This version treats on-plane
        // vertices as inside and may emit a duplicate at an on-plane crossing.
        if ( ((tu <= 0.0) && (tv > 0.0)) || ((tu > 0.0) && (tv <= 0.0)) ) {
            // Edge crosses plane; add intersection point to q
            // Signed plane measures give the edge parameter and linear
            // intersection ([SUTH1974b], p. 36, eqs. 4-5).
            const double t = tu / (tu - tv);
            double w[PolygonVertex::NUMBER_OF_ATTRIBUTES] = {};
            unsigned long maskBits = mask;
            for ( int attributeIndex = 0; maskBits != 0 && attributeIndex < PolygonVertex::NUMBER_OF_ATTRIBUTES;
                  attributeIndex++, maskBits >>= 1 ) {
                if ( maskBits & 1UL ) {
                    const double uCoord = u.getCoord(attributeIndex);
                    const double vCoord = v.getCoord(attributeIndex);
                    w[attributeIndex] = uCoord + t * (vCoord - uCoord);
                }
            }
            ConvexWindowClipper::checkRoomForVertex(qn);
            q[qn++] = PolygonVertex::fromAttributes(w);
        }
        if ( tv <= 0.0 ) {
            // Vertex v is in, copy it to q
            ConvexWindowClipper::checkRoomForVertex(qn);
            q[qn++] = v;
        }
        previousVertexIndex = currentVertexIndex;
        tu = tv;
    }
    return ScreenPolygon(qn, mask, q);
}

/**
Clip the polygon successively against the given half-spaces, in their order
([SUTH1974b], p. 36). Unlike the streaming reentrant pipeline of the paper,
this version stores each complete intermediate polygon and reuses the same
single-plane routine. The polygon must leave room for one more vertex per
half-space.
*/
ScreenPolygon
ConvexWindowClipper::clipToHalfSpaces(const ScreenPolygon &polygon, const HomogeneousHalfSpace *halfSpaces, int numberOfHalfSpaces) {
    ScreenPolygon clipped = polygon;
    for ( int i = 0; i < numberOfHalfSpaces; i++ ) {
        clipped = ConvexWindowClipper::clipToHalfSpace(clipped, halfSpaces[i]);
    }
    return clipped;
}
