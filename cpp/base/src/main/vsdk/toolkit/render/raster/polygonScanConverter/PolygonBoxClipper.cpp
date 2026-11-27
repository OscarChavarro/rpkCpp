//= References:                                                             =
//= [HECK1990] Heckbert, Paul S. "Generic Convex Polygon Scan Conversion   =
//=          and Clipping", Graphics Gems, ed. Andrew S. Glassner,         =
//=          Academic Press, 1990, section II.5, pp. 84-86.                 =
//= [SUTH1974b] Sutherland, Ivan E.; Hodgman, Gary W. "Reentrant Polygon  =
//=          Clipping", Communications of the ACM 17(1), 32-42, 1974.     =
//=          DOI: 10.1145/360767.360802.                                  =

/**
Generic Convex Polygon Scan Conversion and Clipping by Paul Heckbert
from "Graphics Gems", Academic Press, 1990

Homogeneous 3-D convex polygon clipper Paul Heckbert 1985, Dec 1989

Note that original algorithm is available at
https://github.com/erich666/GraphicsGems/tree/master/gems/PolyScan
*/

#include "java/lang/System.h"
#include "vsdk/toolkit/environment/geometry/geometricProcessing/convexPolygonClipper/ConvexWindowClipper.h"
#include "vsdk/toolkit/render/raster/polygonScanConverter/PolygonBoxClipper.h"

/**
Clip the convex polygon p1 to the screen space box
using the homogeneous screen coordinates (sx, sy, sz, sw) of each vertex,
testing if v->sx/v->sw > box.getMinX() and v->sx/v->sw < box.getMaxX(),
and similar tests for y and z, for each vertex v of the polygon.
If polygon is entirely inside box, then POLY_CLIP_IN is returned.
If polygon is entirely outside box, then POLY_CLIP_OUT is returned with the
empty polygon. Otherwise, if the polygon is cut by the box, POLY_CLIP_PARTIAL
is returned with the clipped polygon.

Given an n-gon as input, clipping against 6 planes could generate an
(n+6)gon, so ScreenPolygon::MAXIMUM_SIDES_PER_POLYGON must be big enough to
allow that.
*/
PolygonClipOutcome
PolygonBoxClipper::clipToBox(const ScreenPolygon &polygon, const PolygonBox &box) {
    const int n = polygon.getNumberOfVertices();
    int x0out = 0;
    int x1out = 0;
    int y0out = 0;
    int y1out = 0;
    int z0out = 0;
    int z1out = 0;

    if ( n + 6 > ScreenPolygon::MAXIMUM_SIDES_PER_POLYGON ) {
        java::System::err.printf("polyClipToBox: too many vertices: %d (max=%d-6)\n",
                n, ScreenPolygon::MAXIMUM_SIDES_PER_POLYGON);
        java::System::exit(1);
    }

    // Homogeneous plane tests precede division by w ([SUTH1974b], p. 33).
    // This clip box uses [-w, w] on z as well as x and y; the paper uses
    // [0, w] for z. Counts also permit trivial acceptance and rejection.
    for ( int i = 0; i < n; i++ ) {
        const PolygonVertex &vertex = polygon.getVertex(i);
        if ( vertex.getSx() < box.getMinX() * vertex.getSw() ) {
            // Out on left
            x0out++;
        }
        if ( vertex.getSx() > box.getMaxX() * vertex.getSw() ) {
            // Out on right
            x1out++;
        }
        if ( vertex.getSy() < box.getMinY() * vertex.getSw() ) {
            // Out on top
            y0out++;
        }
        if ( vertex.getSy() > box.getMaxY() * vertex.getSw() ) {
            // Out on bottom
            y1out++;
        }
        if ( vertex.getSz() < box.getMinZ() * vertex.getSw() ) {
            // Out on near
            z0out++;
        }
        if ( vertex.getSz() > box.getMaxZ() * vertex.getSw() ) {
            // Out on far
            z1out++;
        }
    }

    // Check if all vertices inside
    if ( x0out + x1out + y0out + y1out + z0out + z1out == 0 ) {
        return PolygonClipOutcome(POLY_CLIP_IN, polygon);
    }

    // Check if all vertices are "outside" any of the six planes
    if ( x0out == n || x1out == n || y0out == n ||
         y1out == n || z0out == n || z1out == n ) {
        return PolygonClipOutcome(POLY_CLIP_OUT, ScreenPolygon());
    }

    // Only the planes that some vertex lies outside of cut the polygon:
    // pass those, in a fixed order, to the convex window clipper
    HomogeneousHalfSpace halfSpaces[6];
    int numberOfHalfSpaces = 0;
    if ( x0out ) {
        halfSpaces[numberOfHalfSpaces++] = HomogeneousHalfSpace(-1.0, 0.0, 0.0, box.getMinX());
    }
    if ( x1out ) {
        halfSpaces[numberOfHalfSpaces++] = HomogeneousHalfSpace(1.0, 0.0, 0.0, -box.getMaxX());
    }
    if ( y0out ) {
        halfSpaces[numberOfHalfSpaces++] = HomogeneousHalfSpace(0.0, -1.0, 0.0, box.getMinY());
    }
    if ( y1out ) {
        halfSpaces[numberOfHalfSpaces++] = HomogeneousHalfSpace(0.0, 1.0, 0.0, -box.getMaxY());
    }
    if ( z0out ) {
        halfSpaces[numberOfHalfSpaces++] = HomogeneousHalfSpace(0.0, 0.0, -1.0, box.getMinZ());
    }
    if ( z1out ) {
        halfSpaces[numberOfHalfSpaces++] = HomogeneousHalfSpace(0.0, 0.0, 1.0, -box.getMaxZ());
    }
    return PolygonClipOutcome(POLY_CLIP_PARTIAL,
        ConvexWindowClipper::clipToHalfSpaces(polygon, halfSpaces, numberOfHalfSpaces));
}
