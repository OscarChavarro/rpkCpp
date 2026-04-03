/**
Definitions for polygon package
*/

package vsdk.toolkit.render.sgl;

/**
Mask is an interpolation mask whose kth bit indicates whether the kth
double in a Poly_vert is relevant.
For example, if the valid attributes are sx, sy, and sz, then set
mask = Poly::mask(offsetof(PolygonVertex, sx)) |
       Poly::mask(offsetof(PolygonVertex, sy)) |
       Poly::mask(offsetof(PolygonVertex, sz));
*/

/**
Generic Convex Polygon Scan Conversion and Clipping by Paul Heckbert
from "Graphics Gems", Academic Press, 1990

Homogeneous 3-D convex polygon clipper Paul Heckbert 1985, Dec 1989

Note that original algorithm is available at
https://github.com/erich666/GraphicsGems/tree/master/gems/PolyScan
*/

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

import vsdk.toolkit.common.linealAlgebra.Numeric;

public class Poly {
    private static void copyVertex(PolygonVertex dst, PolygonVertex src) {
        dst.sx = src.sx;
        dst.sy = src.sy;
        dst.sz = src.sz;
        dst.sw = src.sw;
        dst.x = src.x;
        dst.y = src.y;
        dst.z = src.z;
        dst.u = src.u;
        dst.v = src.v;
        dst.r = src.r;
        dst.g = src.g;
        dst.b = src.b;
    }

    private static void polygonSwap(Polygon a, Polygon b) {
        int temporaryN = a.n;
        long temporaryMask = a.mask;
        PolygonVertex[] temporaryVertices = a.vertices;

        a.n = b.n;
        a.mask = b.mask;
        a.vertices = b.vertices;

        b.n = temporaryN;
        b.mask = temporaryMask;
        b.vertices = temporaryVertices;
    }

    /**
Clip convex polygon p against a plane,
copying the portion satisfying sign*s[index] < k*sw into q,
where s[index] is one of the vertex coordinates selected by index.
index is a coordinate selector (sx, sy, or sz for clipping).
Thus, to clip against xMin, use
polyClipToHalfSpace(p, q, X_INDEX, -1.0, -xMin);
and to clip against xMax, use
polyClipToHalfSpace(p, q, X_INDEX, 1.0,  xMax);
*/
    private static void setPolygonVertexCoord(PolygonVertex vertex, int index, double value) {
        switch (index) {
            case 0:
                vertex.sx = value;
                break;
            case 1:
                vertex.sy = value;
                break;
            case 2:
                vertex.sz = value;
                break;
            case 3:
                vertex.sw = value;
                break;
            case 4:
                vertex.x = value;
                break;
            case 5:
                vertex.y = value;
                break;
            case 6:
                vertex.z = value;
                break;
            case 7:
                vertex.u = value;
                break;
            case 8:
                vertex.v = value;
                break;
            case 9:
                vertex.r = value;
                break;
            case 10:
                vertex.g = value;
                break;
            case 11:
                vertex.b = value;
                break;
            default:
                break;
        }
    }

    private static void clipToHalfSpace(Polygon p, Polygon q, int index, double sign, double k) {
        q.n = 0;
        q.mask = p.mask;

        // Start with u=vert[n-1], v=vert[0]
        int previousVertexIndex = p.n - 1;
        double tu = sign * p.vertices[previousVertexIndex].getCoord(index) - p.vertices[previousVertexIndex].sw * k;
        for (int currentVertexIndex = 0; currentVertexIndex < p.n; currentVertexIndex++) {
            // On old polygon (p), u is previous vertex, v is current vertex
            // tv is negative if vertex v is in
            PolygonVertex u = p.vertices[previousVertexIndex];
            PolygonVertex v = p.vertices[currentVertexIndex];
            double tv = sign * v.getCoord(index) - v.sw * k;
            if (((tu <= 0.0) && (tv > 0.0)) || ((tu > 0.0) && (tv <= 0.0))) {
                // Edge crosses plane; add intersection point to q
                double t = tu / (tu - tv);
                PolygonVertex w = q.vertices[q.n];
                long maskBits = p.mask;
                for (int attributeIndex = 0; maskBits != 0; attributeIndex++, maskBits >>= 1) {
                    if ((maskBits & 1L) != 0) {
                        double uCoord = u.getCoord(attributeIndex);
                        double vCoord = v.getCoord(attributeIndex);
                        Poly.setPolygonVertexCoord(w, attributeIndex, uCoord + t * (vCoord - uCoord));
                    }
                }
                q.n++;
            }
            if (tv <= 0.0) {
                // Vertex v is in, copy it to q
                copyVertex(q.vertices[q.n], v);
                q.n++;
            }
            previousVertexIndex = currentVertexIndex;
            tu = tv;
        }
    }

    private static void clipAndSwap(int elementIndex, double sign, double k, Polygon p, Polygon q, Polygon p1) {
        Poly.clipToHalfSpace(p, q, elementIndex, sign, sign * k);
        if (q.n == 0) {
            p1.n = 0;
        }
        Poly.polygonSwap(p, q);
    }

    public static long mask(long elementOffset) {
        return 1L << (elementOffset / Double.BYTES);
    }

    /**
Clip the convex polygon p1 to the screen space box
using the homogeneous screen coordinates (sx, sy, sz, sw) of each vertex,
testing if v->sx/v->sw > box->x0 and v->sx/v->sw < box->x1,
and similar tests for y and z, for each vertex v of the polygon.
If polygon is entirely inside box, then POLY_CLIP_IN is returned.
If polygon is entirely outside box, then POLY_CLIP_OUT is returned.
Otherwise, if the polygon is cut by the box, p1 is modified and
POLY_CLIP_PARTIAL is returned.

Given an n-gon as input, clipping against 6 planes could generate an
(n+6)gon, so POLY_N_MAX in poly.h must be big enough to allow that.
*/
    public static int clipToBox(Polygon p1, PolygonBox box) {
        int x0out = 0;
        int x1out = 0;
        int y0out = 0;
        int y1out = 0;
        int z0out = 0;
        int z1out = 0;
        Polygon p2 = new Polygon();

        if (p1.n + 6 > PolygonClipResultInfo.MAXIMUM_SIDES_PER_POLYGON) {
            System.err.printf("polyClipToBox: too many vertices: %d (max=%d-6)\n",
                p1.n, PolygonClipResultInfo.MAXIMUM_SIDES_PER_POLYGON);
            System.exit(1);
        }

        // Count vertices "outside" with respect to each of the six planes
        for (int i = 0; i < p1.n; i++) {
            PolygonVertex vertex = p1.vertices[i];
            if (vertex.sx < box.x0 * vertex.sw) {
                // Out on left
                x0out++;
            }
            if (vertex.sx > box.x1 * vertex.sw) {
                // Out on right
                x1out++;
            }
            if (vertex.sy < box.y0 * vertex.sw) {
                // Out on top
                y0out++;
            }
            if (vertex.sy > box.y1 * vertex.sw) {
                // Out on bottom
                y1out++;
            }
            if (vertex.sz < box.z0 * vertex.sw) {
                // Out on near
                z0out++;
            }
            if (vertex.sz > box.z1 * vertex.sw) {
                // Out on far
                z1out++;
            }
        }

        // Check if all vertices inside
        if (x0out + x1out + y0out + y1out + z0out + z1out == 0) {
            return PolygonClipResult.POLY_CLIP_IN;
        }

        // Check if all vertices are "outside" any of the six planes
        if (x0out == p1.n || x1out == p1.n || y0out == p1.n ||
            y1out == p1.n || z0out == p1.n || z1out == p1.n) {
            p1.n = 0;
            return PolygonClipResult.POLY_CLIP_OUT;
        }

        // Now clip against each of the planes that might cut the polygon,
        // at each step toggling between polygons p1 and p2
        Polygon p = p1;
        Polygon q = p2;
        if (x0out != 0) {
            Poly.clipAndSwap(0 /*sx*/, -1.0, box.x0, p, q, p1);
        }
        if (x1out != 0) {
            Poly.clipAndSwap(0 /*sx*/, 1.0, box.x1, p, q, p1);
        }
        if (y0out != 0) {
            Poly.clipAndSwap(1 /*sy*/, -1.0, box.y0, p, q, p1);
        }
        if (y1out != 0) {
            Poly.clipAndSwap(1 /*sy*/, 1.0, box.y1, p, q, p1);
        }
        if (z0out != 0) {
            Poly.clipAndSwap(2 /*sz*/, -1.0, box.z0, p, q, p1);
        }
        if (z1out != 0) {
            Poly.clipAndSwap(2 /*sz*/, 1.0, box.z1, p, q, p1);
        }

        // If result ended up in p2 then copy it to p1
        if (p == p2) {
            p1.n = p2.n;
            p1.mask = p2.mask;
            for (int i = 0; i < p2.n; i++) {
                copyVertex(p1.vertices[i], p2.vertices[i]);
            }
        }
        return PolygonClipResult.POLY_CLIP_PARTIAL;
    }

    /**
Put intersection of line Y = y + 0.5 with edge between positions
p1 and p2 in p, put change with respect to y in dp
*/
    private static void incrementalizeYFlat(PolygonVertex p1, PolygonVertex p2, PolygonVertex p, PolygonVertex dp, int y) {
        double dy = p2.sy - p1.sy;
        if (dy == 0.0) {
            dy = 1.0;
        }
        double frac = y + 0.5 - p1.sy;

        // Interpolate only sx
        dp.sx = (p2.sx - p1.sx) / dy;
        p.sx = p1.sx + dp.sx * frac;
    }

    private static void incrementFlat(PolygonVertex p, PolygonVertex dp) {
        // Interpolate only sx
        p.sx += dp.sx;
    }

    /**
Output scanline by sampling polygon at Y = y + 0.5
*/
    private static void scanlineFlat(SglContext sglContext, int y, PolygonVertex l, PolygonVertex r, Window win) {
        int lx = (int)Math.ceil(l.sx - 0.5);
        if (lx < win.x0) {
            lx = win.x0;
        }

        int rx = (int)Math.floor(r.sx - 0.5);
        if (rx > win.x1) {
            rx = win.x1;
        }
        if (lx > rx) {
            return;
        }

        int rowStart = y * sglContext.width;
        for (int x = lx; x <= rx; x++) {
            // Scan in x, generating pixels
            int pixelIndex = rowStart + x;
            if (sglContext.pixelData == SglPixelContent.PATCH_POINTER) {
                sglContext.patchBuffer[pixelIndex] = sglContext.currentPatch;
            }
            else if (sglContext.pixelData == SglPixelContent.ELEMENT_POINTER) {
                sglContext.galerkinElementBuffer[pixelIndex] = sglContext.currentGalerkinElement;
            }
            else {
                sglContext.frameBuffer[pixelIndex] = sglContext.currentPixel;
            }
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
    public static void scanFlat(SglContext sglContext, Polygon p, Window win) {
        int i;
        int y;
        PolygonVertex l = new PolygonVertex();
        PolygonVertex r = new PolygonVertex();
        PolygonVertex dl = new PolygonVertex();
        PolygonVertex dr = new PolygonVertex();

        double yMin = Numeric.HUGE_DOUBLE_VALUE;
        int top = -1;
        for (i = 0; i < p.n; i++) {
            // Find top vertex (y positions down)
            if (p.vertices[i].sy < yMin) {
                yMin = p.vertices[i].sy;
                top = i;
            }
        }

        int li = top;
        int ri = top; // Left and right vertex indices
        int rem = p.n; // Number of vertices remaining
        y = (int)Math.ceil(yMin - 0.5); // Current scan line
        int ly = y - 1;
        int ry = y - 1; // Lower end of left & right edges

        while (rem > 0) {
            // Scan in y, activating new edges on left & right
            // as scan line passes over new vertices

            while (ly <= y && rem > 0) {
                // Advance left edge?
                rem--;
                i = li - 1; // Step ccw down left side
                if (i < 0) {
                    i = p.n - 1;
                }
                Poly.incrementalizeYFlat(p.vertices[li], p.vertices[i], l, dl, y);
                ly = (int)Math.floor(p.vertices[i].sy + 0.5);
                li = i;
            }
            while (ry <= y && rem > 0) {
                // Advance right edge?
                rem--;
                i = ri + 1; // Step cw down right edge
                if (i >= p.n) {
                    i = 0;
                }
                Poly.incrementalizeYFlat(p.vertices[ri], p.vertices[i], r, dr, y);
                ry = (int)Math.floor(p.vertices[i].sy + .5);
                ri = i;
            }

            while (y < ly && y < ry) {
                // Do scan lines till end of l or r edge
                if (y >= win.y0 && y <= win.y1) {
                    if (l.sx <= r.sx) {
                        Poly.scanlineFlat(sglContext, y, l, r, win);
                    }
                    else {
                        Poly.scanlineFlat(sglContext, y, r, l, win);
                    }
                }
                y++;
                Poly.incrementFlat(l, dl);
                Poly.incrementFlat(r, dr);
            }
        }
    }

    /**
incrementalizeY: put intersection of line Y = y + 0.5 with edge between positions
p1 and p2 in p, put change with respect to y in dp
*/
    private static void incrementalizeYZ(PolygonVertex p1, PolygonVertex p2, PolygonVertex p, PolygonVertex dp, int y) {
        double dy = p2.sy - p1.sy;
        if (dy == 0.0) {
            dy = 1.0;
        }
        double frac = y + 0.5 - p1.sy;

        // Interpolate only sx and sz (first and third field)
        dp.sx = (p2.sx - p1.sx) / dy;
        p.sx = p1.sx + dp.sx * frac;
        dp.sz = (p2.sz - p1.sz) / dy;
        p.sz = p1.sz + dp.sz * frac;
    }

    private static void incrementZ(PolygonVertex p, PolygonVertex dp) {
        // Increment only sx and sz
        p.sx += dp.sx;
        p.sz += dp.sz;
    }

    /**
Output scanline by sampling polygon at Y = y + 0.5
*/
    private static void scanlineZ(SglContext sglContext, int y, PolygonVertex l, PolygonVertex r, Window win) {
        int lx = (int)Math.ceil(l.sx - 0.5);
        if (lx < win.x0) {
            lx = win.x0;
        }

        int rx = (int)Math.floor(r.sx - 0.5);
        if (rx > win.x1) {
            rx = win.x1;
        }
        if (lx > rx) {
            return;
        }

        double dx = r.sx - l.sx;
        if (dx == 0.0) {
            dx = 1.0;
        }

        double frac = lx + 0.5 - l.sx;
        double dzf = (r.sz - l.sz) / dx;
        long z = (long)(l.sz + dzf * frac);
        long dz = (long)dzf;

        int rowStart = y * sglContext.width;
        int pixelIndex = rowStart + lx;
        for (int x = lx; x <= rx; x++) {
            // Scan in x, generating pixels
            if (z <= sglContext.depthBuffer[pixelIndex]) {
                if (sglContext.pixelData == SglPixelContent.PATCH_POINTER) {
                    sglContext.patchBuffer[pixelIndex] = sglContext.currentPatch;
                }
                else if (sglContext.pixelData == SglPixelContent.ELEMENT_POINTER) {
                    sglContext.galerkinElementBuffer[pixelIndex] = sglContext.currentGalerkinElement;
                }
                else {
                    sglContext.frameBuffer[pixelIndex] = sglContext.currentPixel;
                }
                sglContext.depthBuffer[pixelIndex] = z;
            }
            pixelIndex++;
            z += dz;
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
    public static void scanZ(SglContext sglContext, Polygon p, Window window) {
        int i;

        double yMin = Numeric.HUGE_DOUBLE_VALUE;
        int top = -1;
        for (i = 0; i < p.n; i++) {
            // Find top vertex (y positions down)
            if (p.vertices[i].sy < yMin) {
                yMin = p.vertices[i].sy;
                top = i;
            }
        }

        int li = top; // Left and right vertex indices
        int ri = top;
        int rem = p.n; // Number of vertices remaining
        int y = (int)Math.ceil(yMin - 0.5); // Current scan line
        int ly = y - 1; // Lower end of left & right edges
        int ry = y - 1;

        PolygonVertex l = new PolygonVertex();
        PolygonVertex r = new PolygonVertex();
        PolygonVertex dl = new PolygonVertex();
        PolygonVertex dr = new PolygonVertex();

        while (rem > 0) {
            // Scan in y, activating new edges on left & right
            // as scan line passes over new vertices
            while (ly <= y && rem > 0) {
                // Advance left edge?
                rem--;
                i = li - 1; // Step ccw down left side
                if (i < 0) {
                    i = p.n - 1;
                }
                Poly.incrementalizeYZ(p.vertices[li], p.vertices[i], l, dl, y);
                ly = (int)Math.floor(p.vertices[i].sy + 0.5);
                li = i;
            }
            while (ry <= y && rem > 0) {
                // Advance right edge?
                rem--;
                i = ri + 1; // Step cw down right edge
                if (i >= p.n) {
                    i = 0;
                }
                Poly.incrementalizeYZ(p.vertices[ri], p.vertices[i], r, dr, y);
                ry = (int)Math.floor(p.vertices[i].sy + 0.5);
                ri = i;
            }

            while (y < ly && y < ry) {
                // Do scan lines till end of l or r edge
                if (y >= window.y0 && y <= window.y1) {
                    if (l.sx <= r.sx) {
                        Poly.scanlineZ(sglContext, y, l, r, window);
                    }
                    else {
                        Poly.scanlineZ(sglContext, y, r, l, window);
                    }
                }
                y++;
                Poly.incrementZ(l, dl);
                Poly.incrementZ(r, dr);
            }
        }
    }
}
