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

#include "SGL/poly.h"

/**
Put intersection of line Y = y + 0.5 with edge between positions
p1 and p2 in p, put change with respect to y in dp
*/
static void
incrementalizeY(const PolygonVertex &p1, const PolygonVertex &p2, PolygonVertex *p, PolygonVertex *dp, int y) {
    double dy = p2.sy - p1.sy;
    if ( dy == 0.0 ) {
        dy = 1.0;
    }
    const double frac = y + 0.5 - p1.sy;

    // Interpolate only sx
    dp->sx = (p2.sx - p1.sx) / dy;
    p->sx = p1.sx + dp->sx * frac;
}

static void
increment(PolygonVertex *p, const PolygonVertex &dp) {
    // Interpolate only sx
    p->sx += dp.sx;
}

/**
Output scanline by sampling polygon at Y = y + 0.5
*/
static void
scanline(const SGL_CONTEXT *sglContext, int y, const PolygonVertex *l, const PolygonVertex *r, const Window *win) {
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

    const int rowStart = y * sglContext->width;
    for ( int x = lx; x <= rx; x++ ) {
        // Scan in x, generating pixels
        if ( sglContext->pixelData == SglPixelContent::PATCH_POINTER ) {
            sglContext->patchBuffer[rowStart + x] = const_cast<Patch *>(sglContext->currentPatch);
        } else {
            sglContext->frameBuffer[rowStart + x] = sglContext->currentPixel;
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
void
polyScanFlat(SGL_CONTEXT *sglContext, Polygon *p, const Window *win)
{
    int i;
    int ri;
    int ry;
    PolygonVertex l{};
    PolygonVertex r{};
    PolygonVertex dl{};
    PolygonVertex dr{};

    double yMin = Numeric::HUGE_DOUBLE_VALUE;
    int top = -1;
    for ( i = 0; i < p->n; i++ ) {
        // Find top vertex (y positions down)
        if ( p->vertices[i].sy < yMin ) {
            yMin = p->vertices[i].sy;
            top = i;
        }
    }

    int li = ri = top; // Left and right vertex indices
    int rem = p->n; // Number of vertices remaining
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
                i = p->n - 1;
            }
            incrementalizeY(p->vertices[li], p->vertices[i], &l, &dl, y);
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
            incrementalizeY(p->vertices[ri], p->vertices[i], &r, &dr, y);
            ry = static_cast<int>(java::Math::floor(p->vertices[i].sy + .5));
            ri = i;
        }

        while ( y < ly && y < ry ) {
            // Do scan lines till end of l or r edge
            if ( y >= win->y0 && y <= win->y1 ) {
                if ( l.sx <= r.sx ) {
                    scanline(sglContext, y, &l, &r, win);
                } else {
                    scanline(sglContext, y, &r, &l, win);
                }
            }
            y++;
            increment(&l, dl);
            increment(&r, dr);
        }
    }
}
