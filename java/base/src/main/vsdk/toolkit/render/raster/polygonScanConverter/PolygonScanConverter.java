package vsdk.toolkit.render.raster.polygonScanConverter;

import vsdk.toolkit.environment.geometry.geometricProcessing.convexPolygonClipper.PolygonVertex;
import vsdk.toolkit.environment.geometry.geometricProcessing.convexPolygonClipper.ScreenPolygon;

/**
Point-sampled scan conversion of convex polygons after [HECK1990], writing one
payload per covered sample, with or without a depth test. It keeps no state.

Depth values (RasterDepthValue, an unsigned long in the C++ port) are stored
in Java long values and compared as unsigned numbers.

The output reference parameters of the C++ port (sx, dsx, sz, dsz) are
single element arrays here.
*/
public final class PolygonScanConverter {
    public static final long MAXIMUM_DEPTH = 4294967295L;

    private PolygonScanConverter() {
    }

    //= PolygonScanFlat ======================================================

    /*
    Generic Convex Polygon Scan Conversion and Clipping
    by Paul Heckbert
    from "Graphics Gems", Academic Press, 1990

    Does no interpolations, for flat shading without Z buffering etc... PhB

    Point-sampled scan conversion of convex polygons

    Paul Heckbert 1985, Dec 1989

    Note that original algorithm is available at
    https://github.com/erich666/GraphicsGems/tree/master/gems/PolyScan
    */

    /**
    Put intersection of line Y = y + 0.5 with edge between positions
    p1 and p2 in sx, put change with respect to y in dsx
    */
    private static void
    incrementalizeYFlat(PolygonVertex p1, PolygonVertex p2, int y, double[] sx, double[] dsx) {
        double dy = p2.getSy() - p1.getSy();
        if ( dy == 0.0 ) {
            dy = 1.0;
        }
        final double frac = y + 0.5 - p1.getSy();

        // Interpolate only sx
        dsx[0] = (p2.getSx() - p1.getSx()) / dy;
        sx[0] = p1.getSx() + dsx[0] * frac;
    }

    /**
    Output scanline by sampling polygon at Y = y + 0.5
    */
    private static <Sample> void
    scanlineFlat(Sample[] samples, int stride, Sample value, int y, double lSx, double rSx, RasterRectangle2D win) {
        int lx = (int)(Math.ceil(lSx - 0.5));
        if ( lx < win.getMinX() ) {
            lx = win.getMinX();
        }

        int rx = (int)(Math.floor(rSx - 0.5));
        if ( rx > win.getMaxX() ) {
            rx = win.getMaxX();
        }
        if ( lx > rx ) {
            return;
        }

        final int rowStart = y * stride;
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
    public static <Sample> void
    scanFlat(Sample[] samples, int stride, Sample value, ScreenPolygon p, RasterRectangle2D win)
    {
        int i;
        int ri;
        int ry;
        double[] lSx = {0.0}; // Left and right edge positions, and their changes with y
        double[] rSx = {0.0};
        double[] dlSx = {0.0};
        double[] drSx = {0.0};

        double yMin = Double.MAX_VALUE;
        int top = -1;
        for ( i = 0; i < p.getNumberOfVertices(); i++ ) {
            // Find top vertex (y positions down)
            if ( p.getVertex(i).getSy() < yMin ) {
                yMin = p.getVertex(i).getSy();
                top = i;
            }
        }

        ri = top;
        int li = top; // Left and right vertex indices
        int rem = p.getNumberOfVertices(); // Number of vertices remaining
        int y = (int)(Math.ceil(yMin - 0.5)); // Current scan line
        ry = y - 1;
        int ly = y - 1; // Lower end of left & right edges

        while ( rem > 0 ) {
            // Scan in y, activating new edges on left & right
            // as scan line passes over new vertices

            while ( ly <= y && rem > 0 ) {
                // Advance left edge?
                rem--;
                i = li - 1; // Step ccw down left side
                if ( i < 0 ) {
                    i = p.getNumberOfVertices() - 1;
                }
                PolygonScanConverter.incrementalizeYFlat(p.getVertex(li), p.getVertex(i), y, lSx, dlSx);
                ly = (int)(Math.floor(p.getVertex(i).getSy() + 0.5));
                li = i;
            }
            while ( ry <= y && rem > 0 ) {
                // Advance right edge?
                rem--;
                i = ri + 1; // Step cw down right edge
                if ( i >= p.getNumberOfVertices() ) {
                    i = 0;
                }
                PolygonScanConverter.incrementalizeYFlat(p.getVertex(ri), p.getVertex(i), y, rSx, drSx);
                ry = (int)(Math.floor(p.getVertex(i).getSy() + .5));
                ri = i;
            }

            while ( y < ly && y < ry ) {
                // Do scan lines till end of l or r edge
                if ( y >= win.getMinY() && y <= win.getMaxY() ) {
                    if ( lSx[0] <= rSx[0] ) {
                        PolygonScanConverter.scanlineFlat(samples, stride, value, y, lSx[0], rSx[0], win);
                    } else {
                        PolygonScanConverter.scanlineFlat(samples, stride, value, y, rSx[0], lSx[0], win);
                    }
                }
                y++;
                // Interpolate only sx
                lSx[0] += dlSx[0];
                rSx[0] += drSx[0];
            }
        }
    }

    //= PolygonScanZ =========================================================

    /*
    Flat shading + Z buffering
    Generic Convex Polygon Scan Conversion and Clipping
    by Paul Heckbert
    from "Graphics Gems", Academic Press, 1990
    Point-sampled scan conversion of convex polygons

    Paul Heckbert 1985, Dec 1989

    Note that original algorithm is available at
    https://github.com/erich666/GraphicsGems/tree/master/gems/PolyScan
    */

    /**
    incrementalizeY: put intersection of line Y = y + 0.5 with edge between positions
    p1 and p2 in (sx, sz), put change with respect to y in (dsx, dsz)
    */
    private static void
    incrementalizeYZ(PolygonVertex p1, PolygonVertex p2, int y,
                     double[] sx, double[] dsx, double[] sz, double[] dsz) {
        double dy = p2.getSy() - p1.getSy();
        if ( dy == 0.0 ) {
            dy = 1.0;
        }
        double frac = y + 0.5 - p1.getSy();

        // Interpolate only sx and sz (first and third field)
        dsx[0] = (p2.getSx() - p1.getSx()) / dy;
        sx[0] = p1.getSx() + dsx[0] * frac;
        dsz[0] = (p2.getSz() - p1.getSz()) / dy;
        sz[0] = p1.getSz() + dsz[0] * frac;
    }

    /**
    Depth buffer value of an interpolated depth, clamped to [0, MAXIMUM_DEPTH]
    (NaN gives 0)
    */
    private static long
    depthValue(double depth) {
        if ( !(depth > 0.0) ) {
            return 0;
        }
        if ( depth >= (double)PolygonScanConverter.MAXIMUM_DEPTH ) {
            return PolygonScanConverter.MAXIMUM_DEPTH;
        }
        return (long)depth;
    }

    /**
    Output scanline by sampling polygon at Y = y + 0.5
    */
    private static <Sample> void
    scanlineZ(Sample[] samples, long[] depth, int stride, Sample value, int y,
              double lSx, double lSz, double rSx, double rSz, RasterRectangle2D win) {
        int lx = (int)(Math.ceil(lSx - 0.5));
        if ( lx < win.getMinX() ) {
            lx = win.getMinX();
        }

        int rx = (int)(Math.floor(rSx - 0.5));
        if ( rx > win.getMaxX() ) {
            rx = win.getMaxX();
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

        final int rowStart = y * stride;
        int pixelIndex = rowStart + lx;
        for ( int x = lx; x <= rx; x++ ) {
            // Scan in x, generating pixels
            final long z = PolygonScanConverter.depthValue(zf);
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
    public static <Sample> void
    scanZ(Sample[] samples, long[] depth, int stride, Sample value, ScreenPolygon p, RasterRectangle2D window)
    {
        int i;

        double yMin = Double.MAX_VALUE;
        int top = -1;
        for ( i = 0; i < p.getNumberOfVertices(); i++ ) {
            // Find top vertex (y positions down)
            if ( p.getVertex(i).getSy() < yMin ) {
                yMin = p.getVertex(i).getSy();
                top = i;
            }
        }

        int li = top; // Left and right vertex indices
        int ri = top;
        int rem = p.getNumberOfVertices(); // Number of vertices remaining
        int y = (int)(Math.ceil(yMin - 0.5)); // Current scan line
        int ly = y - 1; // Lower end of left & right edges
        int ry = y - 1;

        double[] lSx = {0.0}; // Left and right edge positions, and their changes with y
        double[] lSz = {0.0};
        double[] rSx = {0.0};
        double[] rSz = {0.0};
        double[] dlSx = {0.0};
        double[] dlSz = {0.0};
        double[] drSx = {0.0};
        double[] drSz = {0.0};

        while ( rem > 0 ) {
            // Scan in y, activating new edges on left & right
            // as scan line passes over new vertices
            while ( ly <= y && rem > 0 ) {
                // Advance left edge?
                rem--;
                i = li - 1; // Step ccw down left side
                if ( i < 0 ) {
                    i = p.getNumberOfVertices() - 1;
                }
                PolygonScanConverter.incrementalizeYZ(p.getVertex(li), p.getVertex(i), y, lSx, dlSx, lSz, dlSz);
                ly = (int)(Math.floor(p.getVertex(i).getSy() + 0.5));
                li = i;
            }
            while ( ry <= y && rem > 0 ) {
                // Advance right edge?
                rem--;
                i = ri + 1; // Step cw down right edge
                if ( i >= p.getNumberOfVertices() ) {
                    i = 0;
                }
                PolygonScanConverter.incrementalizeYZ(p.getVertex(ri), p.getVertex(i), y, rSx, drSx, rSz, drSz);
                ry = (int)(Math.floor(p.getVertex(i).getSy() + 0.5));
                ri = i;
            }

            while ( y < ly && y < ry ) {
                // Do scan lines till end of l or r edge
                if ( y >= window.getMinY() && y <= window.getMaxY() ) {
                    if ( lSx[0] <= rSx[0] ) {
                        PolygonScanConverter.scanlineZ(samples, depth, stride, value, y, lSx[0], lSz[0], rSx[0], rSz[0], window);
                    } else {
                        PolygonScanConverter.scanlineZ(samples, depth, stride, value, y, rSx[0], rSz[0], lSx[0], lSz[0], window);
                    }
                }
                y++;
                // Increment only sx and sz
                lSx[0] += dlSx[0];
                lSz[0] += dlSz[0];
                rSx[0] += drSx[0];
                rSz[0] += drSz[0];
            }
        }
    }
}
