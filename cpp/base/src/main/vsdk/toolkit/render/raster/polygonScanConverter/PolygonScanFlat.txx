/**
Generic Convex Polygon Scan Conversion and Clipping
by Paul Heckbert
from "Graphics Gems", Academic Press, 1990

Does no interpolations, for flat shading without Z buffering etc... PhB

Point-sampled scan conversion of convex polygons

Paul Heckbert 1985, Dec 1989

Note that original algorithm is available at
https://github.com/erich666/GraphicsGems/tree/master/gems/PolyScan
*/

#include "java/lang/Double.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/render/raster/polygonScanConverter/PolygonScanConverter.h"
/**
Put intersection of line Y = y + 0.5 with edge between positions
p1 and p2 in sx, put change with respect to y in dsx
*/
inline void
PolygonScanConverter::incrementalizeYFlat(const PolygonVertex &p1, const PolygonVertex &p2, int y, double &sx, double &dsx) {
    double dy = p2.getSy() - p1.getSy();
    if ( dy == 0.0 ) {
        dy = 1.0;
    }
    const double frac = y + 0.5 - p1.getSy();

    // Interpolate only sx
    dsx = (p2.getSx() - p1.getSx()) / dy;
    sx = p1.getSx() + dsx * frac;
}

/**
Output scanline by sampling polygon at Y = y + 0.5
*/
template<class Sample>
void
PolygonScanConverter::scanlineFlat(Sample *samples, int stride, Sample value, int y, double lSx, double rSx, const RasterRectangle2D *win) {
    int lx = static_cast<int>(java::Math::ceil(lSx - 0.5));
    if ( lx < win->getMinX() ) {
        lx = win->getMinX();
    }

    int rx = static_cast<int>(java::Math::floor(rSx - 0.5));
    if ( rx > win->getMaxX() ) {
        rx = win->getMaxX();
    }
    if ( lx > rx ) {
        return;
    }

    const int rowStart = y * stride;
    for ( int x = lx; x <= rx; x++ ) {
        // Scan in x, generating pixels
        samples[rowStart + x] = value;
    }
}

/**
Scan convert a polygon, calling pixelProc at each pixel with an
interpolated Poly_vert structure.  Polygon can be clockwise or ccw.
Polygon is clipped in 2-D to win, the screen space window.

Scan conversion is done on the basis of Poly_vert fields sx and sy.
These two must always be interpolated, and only they have special meaning
to this code; any other fields are blindly interpolated regardless of
their semantics.

The pixelProc subroutine takes the arguments:

pixelProc(x, y, point)
int x, y;
Poly_vert *point;

All the fields of point indicated by p->mask will be valid inside pixelProc
except sx and sy.  If they were computed, they would have values
sx=x+.5 and sy=y+.5, since sampling is done at pixel centers

p: polygon
win: 2-D screen space clipping window
*/
template<class Sample>
void
PolygonScanConverter::scanFlat(Sample *samples, int stride, Sample value, const ScreenPolygon *p, const RasterRectangle2D *win)
{
    int i;
    int ri;
    int ry;
    double lSx = 0.0; // Left and right edge positions, and their changes with y
    double rSx = 0.0;
    double dlSx = 0.0;
    double drSx = 0.0;

    double yMin = java::Double::MAX_VALUE;
    int top = -1;
    for ( i = 0; i < p->getNumberOfVertices(); i++ ) {
        // Find top vertex (y positions down)
        if ( p->getVertex(i).getSy() < yMin ) {
            yMin = p->getVertex(i).getSy();
            top = i;
        }
    }

    int li = ri = top; // Left and right vertex indices
    int rem = p->getNumberOfVertices(); // Number of vertices remaining
    int y = static_cast<int>(java::Math::ceil(yMin - 0.5)); // Current scan line
    int ly = ry = y - 1; // Lower end of left & right edges

    while ( rem > 0 ) {
        // Scan in y, activating new edges on left & right
        // as scan line passes over new vertices

        while ( ly <= y && rem > 0 ) {
            // Advance left edge?
            rem--;
            i = li - 1; // Step ccw down left side
            if ( i < 0 ) {
                i = p->getNumberOfVertices() - 1;
            }
            PolygonScanConverter::incrementalizeYFlat(p->getVertex(li), p->getVertex(i), y, lSx, dlSx);
            ly = static_cast<int>(java::Math::floor(p->getVertex(i).getSy() + 0.5));
            li = i;
        }
        while ( ry <= y && rem > 0 ) {
            // Advance right edge?
            rem--;
            i = ri + 1; // Step cw down right edge
            if ( i >= p->getNumberOfVertices() ) {
                i = 0;
            }
            PolygonScanConverter::incrementalizeYFlat(p->getVertex(ri), p->getVertex(i), y, rSx, drSx);
            ry = static_cast<int>(java::Math::floor(p->getVertex(i).getSy() + .5));
            ri = i;
        }

        while ( y < ly && y < ry ) {
            // Do scan lines till end of l or r edge
            if ( y >= win->getMinY() && y <= win->getMaxY() ) {
                if ( lSx <= rSx ) {
                    PolygonScanConverter::scanlineFlat(samples, stride, value, y, lSx, rSx, win);
                } else {
                    PolygonScanConverter::scanlineFlat(samples, stride, value, y, rSx, lSx, win);
                }
            }
            y++;
            // Interpolate only sx
            lSx += dlSx;
            rSx += drSx;
        }
    }
}
