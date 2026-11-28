import { Double } from "../../../../../java/lang/Double.js";
import { PolygonVertex } from "../../../environment/geometry/geometricProcessing/convexPolygonClipper/PolygonVertex.js";
import { ScreenPolygon } from "../../../environment/geometry/geometricProcessing/convexPolygonClipper/ScreenPolygon.js";
import { RasterRectangle2D } from "./RasterRectangle2D.js";

/**
Depth sample. C++ stores it as an `unsigned long`; here it is an integer valued
number, exact in the Float64Array depth planes up to MAXIMUM_DEPTH.
*/
export type RasterDepthValue = number;

/**
Point-sampled scan conversion of convex polygons after [HECK1990], writing one
payload per covered sample, with or without a depth test. It keeps no state. The
generic implementations of scanFlat and scanZ follow the C++ PolygonScanFlat.txx
and PolygonScanZ.txx.

Output parameters (`sx`, `dsx`, `sz`, `dsz`) are single element arrays, as in
the other out-parameter methods of this port.
*/
export class PolygonScanConverter {
    public static readonly MAXIMUM_DEPTH: RasterDepthValue = 4294967295;

    private constructor() {}

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
    private static incrementalizeYFlat(
        p1: PolygonVertex,
        p2: PolygonVertex,
        y: number,
        sx: number[],
        dsx: number[],
    ): void {
        let dy = p2.getSy() - p1.getSy();
        if (dy === 0.0) {
            dy = 1.0;
        }
        const frac = y + 0.5 - p1.getSy();

        // Interpolate only sx
        dsx[0] = (p2.getSx() - p1.getSx()) / dy;
        sx[0] = p1.getSx() + dsx[0] * frac;
    }

    /**
    Output scanline by sampling polygon at Y = y + 0.5
    */
    private static scanlineFlat<Sample>(
        samples: Sample[],
        stride: number,
        value: Sample,
        y: number,
        lSx: number,
        rSx: number,
        win: RasterRectangle2D,
    ): void {
        let lx = Math.ceil(lSx - 0.5);
        if (lx < win.getMinX()) {
            lx = win.getMinX();
        }

        let rx = Math.floor(rSx - 0.5);
        if (rx > win.getMaxX()) {
            rx = win.getMaxX();
        }
        if (lx > rx) {
            return;
        }

        const rowStart = y * stride;
        for (let x = lx; x <= rx; x++) {
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
    public static scanFlat<Sample>(
        samples: Sample[],
        stride: number,
        value: Sample,
        p: ScreenPolygon,
        win: RasterRectangle2D,
    ): void {
        let i: number;
        let ri: number;
        let ry: number;
        const lSx: number[] = [0.0]; // Left and right edge positions, and their changes with y
        const rSx: number[] = [0.0];
        const dlSx: number[] = [0.0];
        const drSx: number[] = [0.0];

        let yMin = Double.MAX_VALUE;
        let top = -1;
        for (i = 0; i < p.getNumberOfVertices(); i++) {
            // Find top vertex (y positions down)
            if (p.getVertex(i).getSy() < yMin) {
                yMin = p.getVertex(i).getSy();
                top = i;
            }
        }

        let li = (ri = top); // Left and right vertex indices
        let rem = p.getNumberOfVertices(); // Number of vertices remaining
        let y = Math.ceil(yMin - 0.5); // Current scan line
        let ly = (ry = y - 1); // Lower end of left & right edges

        while (rem > 0) {
            // Scan in y, activating new edges on left & right
            // as scan line passes over new vertices

            while (ly <= y && rem > 0) {
                // Advance left edge?
                rem--;
                i = li - 1; // Step ccw down left side
                if (i < 0) {
                    i = p.getNumberOfVertices() - 1;
                }
                PolygonScanConverter.incrementalizeYFlat(p.getVertex(li), p.getVertex(i), y, lSx, dlSx);
                ly = Math.floor(p.getVertex(i).getSy() + 0.5);
                li = i;
            }
            while (ry <= y && rem > 0) {
                // Advance right edge?
                rem--;
                i = ri + 1; // Step cw down right edge
                if (i >= p.getNumberOfVertices()) {
                    i = 0;
                }
                PolygonScanConverter.incrementalizeYFlat(p.getVertex(ri), p.getVertex(i), y, rSx, drSx);
                ry = Math.floor(p.getVertex(i).getSy() + 0.5);
                ri = i;
            }

            while (y < ly && y < ry) {
                // Do scan lines till end of l or r edge
                if (y >= win.getMinY() && y <= win.getMaxY()) {
                    if (lSx[0]! <= rSx[0]!) {
                        PolygonScanConverter.scanlineFlat(samples, stride, value, y, lSx[0]!, rSx[0]!, win);
                    } else {
                        PolygonScanConverter.scanlineFlat(samples, stride, value, y, rSx[0]!, lSx[0]!, win);
                    }
                }
                y++;
                // Interpolate only sx
                lSx[0] = lSx[0]! + dlSx[0]!;
                rSx[0] = rSx[0]! + drSx[0]!;
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
    private static incrementalizeYZ(
        p1: PolygonVertex,
        p2: PolygonVertex,
        y: number,
        sx: number[],
        dsx: number[],
        sz: number[],
        dsz: number[],
    ): void {
        let dy = p2.getSy() - p1.getSy();
        if (dy === 0.0) {
            dy = 1.0;
        }
        const frac = y + 0.5 - p1.getSy();

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
    private static depthValue(depth: number): RasterDepthValue {
        if (!(depth > 0.0)) {
            return 0;
        }
        if (depth >= PolygonScanConverter.MAXIMUM_DEPTH) {
            return PolygonScanConverter.MAXIMUM_DEPTH;
        }
        return Math.trunc(depth);
    }

    /**
    Output scanline by sampling polygon at Y = y + 0.5
    */
    private static scanlineZ<Sample>(
        samples: Sample[],
        depth: Float64Array,
        stride: number,
        value: Sample,
        y: number,
        lSx: number,
        lSz: number,
        rSx: number,
        rSz: number,
        win: RasterRectangle2D,
    ): void {
        let lx = Math.ceil(lSx - 0.5);
        if (lx < win.getMinX()) {
            lx = win.getMinX();
        }

        let rx = Math.floor(rSx - 0.5);
        if (rx > win.getMaxX()) {
            rx = win.getMaxX();
        }
        if (lx > rx) {
            return;
        }

        let dx = rSx - lSx;
        if (dx === 0.0) {
            dx = 1.0;
        }

        const frac = lx + 0.5 - lSx;
        const dzf = (rSz - lSz) / dx;
        // The depth is interpolated in double precision and clamped to the depth
        // range when converted (see depthValue): the extrapolation to the first
        // pixel center may leave that range, where converting a negative double
        // to an unsigned type is undefined, and the step of a narrow span may
        // exceed the range of an int.

        let zf = lSz + dzf * frac;

        const rowStart = y * stride;
        let pixelIndex = rowStart + lx;
        for (let x = lx; x <= rx; x++) {
            // Scan in x, generating pixels
            const z = PolygonScanConverter.depthValue(zf);
            if (z <= depth[pixelIndex]!) {
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
    public static scanZ<Sample>(
        samples: Sample[],
        depth: Float64Array,
        stride: number,
        value: Sample,
        p: ScreenPolygon,
        window: RasterRectangle2D,
    ): void {
        let i: number;

        let yMin = Double.MAX_VALUE;
        let top = -1;
        for (i = 0; i < p.getNumberOfVertices(); i++) {
            // Find top vertex (y positions down)
            if (p.getVertex(i).getSy() < yMin) {
                yMin = p.getVertex(i).getSy();
                top = i;
            }
        }

        let li = top; // Left and right vertex indices
        let ri = top;
        let rem = p.getNumberOfVertices(); // Number of vertices remaining
        let y = Math.ceil(yMin - 0.5); // Current scan line
        let ly = y - 1; // Lower end of left & right edges
        let ry = y - 1;

        const lSx: number[] = [0.0]; // Left and right edge positions, and their changes with y
        const lSz: number[] = [0.0];
        const rSx: number[] = [0.0];
        const rSz: number[] = [0.0];
        const dlSx: number[] = [0.0];
        const dlSz: number[] = [0.0];
        const drSx: number[] = [0.0];
        const drSz: number[] = [0.0];

        while (rem > 0) {
            // Scan in y, activating new edges on left & right
            // as scan line passes over new vertices
            while (ly <= y && rem > 0) {
                // Advance left edge?
                rem--;
                i = li - 1; // Step ccw down left side
                if (i < 0) {
                    i = p.getNumberOfVertices() - 1;
                }
                PolygonScanConverter.incrementalizeYZ(p.getVertex(li), p.getVertex(i), y, lSx, dlSx, lSz, dlSz);
                ly = Math.floor(p.getVertex(i).getSy() + 0.5);
                li = i;
            }
            while (ry <= y && rem > 0) {
                // Advance right edge?
                rem--;
                i = ri + 1; // Step cw down right edge
                if (i >= p.getNumberOfVertices()) {
                    i = 0;
                }
                PolygonScanConverter.incrementalizeYZ(p.getVertex(ri), p.getVertex(i), y, rSx, drSx, rSz, drSz);
                ry = Math.floor(p.getVertex(i).getSy() + 0.5);
                ri = i;
            }

            while (y < ly && y < ry) {
                // Do scan lines till end of l or r edge
                if (y >= window.getMinY() && y <= window.getMaxY()) {
                    if (lSx[0]! <= rSx[0]!) {
                        PolygonScanConverter.scanlineZ(
                            samples,
                            depth,
                            stride,
                            value,
                            y,
                            lSx[0]!,
                            lSz[0]!,
                            rSx[0]!,
                            rSz[0]!,
                            window,
                        );
                    } else {
                        PolygonScanConverter.scanlineZ(
                            samples,
                            depth,
                            stride,
                            value,
                            y,
                            rSx[0]!,
                            rSz[0]!,
                            lSx[0]!,
                            lSz[0]!,
                            window,
                        );
                    }
                }
                y++;
                // Increment only sx and sz
                lSx[0] = lSx[0]! + dlSx[0]!;
                lSz[0] = lSz[0]! + dlSz[0]!;
                rSx[0] = rSx[0]! + drSx[0]!;
                rSz[0] = rSz[0]! + drSz[0]!;
            }
        }
    }
}
