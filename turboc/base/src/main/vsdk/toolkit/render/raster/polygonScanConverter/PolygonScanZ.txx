/**
Flat shading + Z buffering
Generic Convex Polygon Scan Conversion and Clipping
by Paul Heckbert
from "Graphics Gems", Academic Press, 1990
Point-sampled scan conversion of convex polygons

Paul Heckbert 1985, Dec 1989

Note that original algorithm is available at
https://github.com/erich666/GraphicsGems/tree/master/gems/PolyScan
*/

#include "java/lang/Double.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/render/raster/polygonScanConverter/PolygonScanConverter.h"
/**
incrementalizeY: put intersection of line Y = y + 0.5 with edge between positions
p1 and p2 in (sx, sz), put change with respect to y in (dsx, dsz)
*/
template <class Sample>
inline void
PolygonScanConverter<Sample>::incrementalizeYZ(const PolygonVertex &p1, const PolygonVertex &p2, int y,
                                       double &sx, double &dsx, double &sz, double &dsz) {
    double dy = p2.getSy() - p1.getSy();
    if ( dy == 0.0 ) {
        dy = 1.0;
    }
    double frac = y + 0.5 - p1.getSy();

    // Interpolate only sx and sz (first and third field)
    dsx = (p2.getSx() - p1.getSx()) / dy;
    sx = p1.getSx() + dsx * frac;
    dsz = (p2.getSz() - p1.getSz()) / dy;
    sz = p1.getSz() + dsz * frac;
}

/**
Depth buffer value of an interpolated depth, clamped to [0, MAXIMUM_DEPTH]
(NaN gives 0)
*/
template <class Sample>
RasterDepthValue
PolygonScanConverter<Sample>::depthValue(double depth) {
    if ( !(depth > 0.0) ) {
        return 0;
    }
    if ( depth >= (double)PolygonScanConverterDepth::MAXIMUM_DEPTH ) {
        return PolygonScanConverterDepth::MAXIMUM_DEPTH;
    }
    return (RasterDepthValue)depth;
}

/**
Output scanline by sampling polygon at Y = y + 0.5
*/
template <class Sample>
void
PolygonScanConverter<Sample>::scanlineZ(Sample *samples, RasterDepthValue *depth, int stride, Sample value, int y,
                                double lSx, double lSz, double rSx, double rSz, const RasterRectangle2D *win) {
    int lx = (int)(Math::ceil(lSx - 0.5));
    if ( lx < win->getMinX() ) {
        lx = win->getMinX();
    }

    int rx = (int)(Math::floor(rSx - 0.5));
    if ( rx > win->getMaxX() ) {
        rx = win->getMaxX();
    }
    if ( lx > rx ) {
        return;
    }

    double dx = rSx - lSx;
    if ( dx == 0.0 ) {
        dx = 1.0;
    }

    double frac = lx + 0.5 - lSx;
    double dzf = (rSz - lSz) / dx;
    // The depth is interpolated in double precision and clamped to the depth
    // range when converted (see depthValue): the extrapolation to the first
    // pixel center may leave that range, where converting a negative double
    // to an unsigned type is undefined, and the step of a narrow span may
    // exceed the range of an int.
    double zf = lSz + dzf * frac;

    const int rowStart = y * stride;
    int pixelIndex = rowStart + lx;
    for ( int x = lx; x <= rx; x++ ) {
        // Scan in x, generating pixels
        const RasterDepthValue z = PolygonScanConverter<Sample>::depthValue(zf);
        if ( z <= depth[pixelIndex] ) {
            samples[pixelIndex] = value;
            depth[pixelIndex] = z;
        }
        pixelIndex++;
        zf += dzf;
    }
}

/**
Scan convert a polygon, calling pixelProc at each pixel with an
interpolated Poly_vert structure.  Polygon can be clockwise or ccw.
Polygon is clipped in 2-D to window, the screen space window.

Scan conversion is done on the basis of Poly_vert fields sx and sy.
These two must always be interpolated, and only they have special meaning
to this code; any other fields are blindly interpolated regardless of
their semantics.

The pixelProc subroutine takes the arguments:

pixelProc(x, y, point)
int x, y;
Poly_vert *point;

All the fields of point indicated by p->mask will be valid inside pixel proc
except sx and sy. If they were computed, they would have values
sx = x + 0.5 and sy = y + 0.5, since sampling is done at pixel centers.
*/
template <class Sample>
void
PolygonScanConverter<Sample>::scanZ(Sample *samples, RasterDepthValue *depth, int stride, Sample value, const ScreenPolygon *p,  const RasterRectangle2D *window)
{
    int i;

    double yMin = Double::MAX_VALUE;
    int top = -1;
    for ( i = 0; i < p->getNumberOfVertices(); i++ ) {
        // Find top vertex (y positions down)
        if ( p->getVertex(i).getSy() < yMin ) {
            yMin = p->getVertex(i).getSy();
            top = i;
        }
    }

    int li = top; // Left and right vertex indices
    int ri = top;
    int rem = p->getNumberOfVertices(); // Number of vertices remaining
    int y = (int)(Math::ceil(yMin - 0.5)); // Current scan line
    int ly = y - 1; // Lower end of left & right edges
    int ry = y - 1;

    double lSx = 0.0; // Left and right edge positions, and their changes with y
    double lSz = 0.0;
    double rSx = 0.0;
    double rSz = 0.0;
    double dlSx = 0.0;
    double dlSz = 0.0;
    double drSx = 0.0;
    double drSz = 0.0;

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
            PolygonScanConverter<Sample>::incrementalizeYZ(p->getVertex(li), p->getVertex(i), y, lSx, dlSx, lSz, dlSz);
            ly = (int)(Math::floor(p->getVertex(i).getSy() + 0.5));
            li = i;
        }
        while ( ry <= y && rem > 0 ) {
            // Advance right edge?
            rem--;
            i = ri + 1; // Step cw down right edge
            if ( i >= p->getNumberOfVertices() ) {
                i = 0;
            }
            PolygonScanConverter<Sample>::incrementalizeYZ(p->getVertex(ri), p->getVertex(i), y, rSx, drSx, rSz, drSz);
            ry = (int)(Math::floor(p->getVertex(i).getSy() + 0.5));
            ri = i;
        }

        while ( y < ly && y < ry ) {
            // Do scan lines till end of l or r edge
            if ( y >= window->getMinY() && y <= window->getMaxY() ) {
                if ( lSx <= rSx ) {
                    PolygonScanConverter<Sample>::scanlineZ(samples, depth, stride, value, y, lSx, lSz, rSx, rSz, window);
                } else {
                    PolygonScanConverter<Sample>::scanlineZ(samples, depth, stride, value, y, rSx, rSz, lSx, lSz, window);
                }
            }
            y++;
            // Increment only sx and sz
            lSx += dlSx;
            lSz += dlSz;
            rSx += drSx;
            rSz += drSz;
        }
    }
}
