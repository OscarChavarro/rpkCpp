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

#include "vsdk/toolkit/render/sgl/Poly.h"
/**
incrementalizeY: put intersection of line Y = y + 0.5 with edge between positions
p1 and p2 in p, put change with respect to y in dp
*/
void
Poly::incrementalizeYZ(const PolygonVertex &p1, const PolygonVertex &p2, PolygonVertex *p, PolygonVertex *dp, int y) {
    double dy = p2.sy - p1.sy;
    if ( dy == 0.0 ) {
        dy = 1.0;
    }
    double frac = y + 0.5 - p1.sy;

    // Interpolate only sx and sz (first and third field)
    dp->sx = (p2.sx - p1.sx) / dy;
    p->sx = p1.sx + dp->sx * frac;
    dp->sz = (p2.sz - p1.sz) / dy;
    p->sz = p1.sz + dp->sz * frac;
}

void
Poly::incrementZ(PolygonVertex *p, const PolygonVertex &dp) {
    // Increment only sx and sz
    p->sx += dp.sx;
    p->sz += dp.sz;
}

/**
Output scanline by sampling polygon at Y = y + 0.5
*/
void
Poly::scanlineZ(const SglContext *sglContext, int y, const PolygonVertex *l, const PolygonVertex *r, const Window *win) {
    int lx = static_cast<int>(java::Math::ceil(l->sx - 0.5));
    if ( lx < win->x0 ) {
        lx = win->x0;
    }

    int rx = static_cast<int>(java::Math::floor(r->sx - 0.5));
    if ( rx > win->x1 ) {
        rx = win->x1;
    }
    if ( lx > rx ) {
        return;
    }

    double dx = r->sx - l->sx;
    if ( dx == 0.0 ) {
        dx = 1.0;
    }

    double frac = lx + 0.5 - l->sx;
    double dzf = (r->sz - l->sz) / dx;
    SGL_Z_VALUE z = static_cast<SGL_Z_VALUE>(l->sz + dzf * frac);
    int dz = static_cast<int>(dzf);

    const int rowStart = y * sglContext->width;
    int pixelIndex = rowStart + lx;
    for ( int x = lx; x <= rx; x++ ) {
        // Scan in x, generating pixels
        if ( z <= sglContext->depthBuffer[pixelIndex] ) {
            if ( sglContext->pixelData == SglPixelContent::PATCH_POINTER ) {
                sglContext->patchBuffer[pixelIndex] = const_cast<Patch *>(sglContext->currentPatch);
            } else if ( sglContext->pixelData == SglPixelContent::ELEMENT_POINTER ) {
                sglContext->galerkinElementBuffer[pixelIndex] =
                    const_cast<Element *>(sglContext->currentGalerkinElement);
            } else {
                sglContext->frameBuffer[pixelIndex] = sglContext->currentPixel;
            }
            sglContext->depthBuffer[pixelIndex] = z;
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
void
Poly::scanZ(SglContext *sglContext, Polygon *p,  const Window *window)
{
    int i;

    double yMin = Numeric::HUGE_DOUBLE_VALUE;
    int top = -1;
    for ( i = 0; i < p->n; i++ ) {
        // Find top vertex (y positions down)
        if ( p->vertices[i].sy < yMin ) {
            yMin = p->vertices[i].sy;
            top = i;
        }
    }

    int li = top; // Left and right vertex indices
    int ri = top;
    int rem = p->n; // Number of vertices remaining
    int y = static_cast<int>(java::Math::ceil(yMin - 0.5)); // Current scan line
    int ly = y - 1; // Lower end of left & right edges
    int ry = y - 1;

    PolygonVertex l{};
    PolygonVertex r{};
    PolygonVertex dl{};
    PolygonVertex dr{};

    while ( rem > 0 ) {
        // Scan in y, activating new edges on left & right
        // as scan line passes over new vertices
        while ( ly <= y && rem > 0 ) {
            // Advance left edge?
            rem--;
            i = li - 1; // Step ccw down left side
            if ( i < 0 ) {
                i = p->n - 1;
            }
            Poly::incrementalizeYZ(p->vertices[li], p->vertices[i], &l, &dl, y);
            ly = static_cast<int>(java::Math::floor(p->vertices[i].sy + 0.5));
            li = i;
        }
        while ( ry <= y && rem > 0 ) {
            // Advance right edge?
            rem--;
            i = ri + 1; // Step cw down right edge
            if ( i >= p->n ) {
                i = 0;
            }
            Poly::incrementalizeYZ(p->vertices[ri], p->vertices[i], &r, &dr, y);
            ry = static_cast<int>(java::Math::floor(p->vertices[i].sy + 0.5));
            ri = i;
        }

        while ( y < ly && y < ry ) {
            // Do scan lines till end of l or r edge
            if ( y >= window->y0 && y <= window->y1 ) {
                if ( l.sx <= r.sx ) {
                    Poly::scanlineZ(sglContext, y, &l, &r, window);
                } else {
                    Poly::scanlineZ(sglContext, y, &r, &l, window);
                }
            }
            y++;
            Poly::incrementZ(&l, dl);
            Poly::incrementZ(&r, dr);
        }
    }
}
