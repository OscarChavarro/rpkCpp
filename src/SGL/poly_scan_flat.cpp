/**
Generic Convex Polygon Scan Conversion and Clipping
by Paul Heckbert
from "Graphics Gems", Academic Press, 1990

Does no interpolations, for flat shading without Z buffering etc... PhB

Point-sampled scan conversion of convex polygons

Paul Heckbert 1985, Dec 1989
*/

#include "SGL/poly.h"
#include "SGL/sgl.h"

/**
Put intersection of line Y = y + 0.5 with edge between positions
p1 and p2 in p, put change with respect to y in dp
*/
static void
incrementalizeY(const double *p1, const double *p2, double *p, double *dp, int y) {
    double dy;
    double frac;

    dy = ((PolygonVertex *) p2)->sy - ((PolygonVertex *) p1)->sy;
    if ( dy == 0.0 ) {
        dy = 1.0;
    }
    frac = y + 0.5 - ((PolygonVertex *) p1)->sy;

    // Interpolate only sx
    *dp = (*p2 - *p1) / dy;
    *p = *p1 + *dp * frac;
}

static void
increment(double *p, const double *dp) {
    // Interpolate only sx
    *p += *dp;
}

/**
Output scanline by sampling polygon at Y = y + 0.5
*/
static void
scanline(SGL_CONTEXT *sglContext, int y, PolygonVertex *l, PolygonVertex *r, Window *win) {
    int x;
    int lx;
    int rx;
    SGL_PIXEL *pix;

    lx = std::ceil(l->sx - 0.5);
    if ( lx < win->x0 ) {
        lx = win->x0;
    }
    rx = std::floor(r->sx - 0.5);
    if ( rx > win->x1 ) {
        rx = win->x1;
    }
    if ( lx > rx ) {
        return;
    }

    pix = sglContext->frameBuffer + y * sglContext->width + lx;
    Patch **patch = sglContext->patchBuffer + lx;
    for ( x = lx; x <= rx; x++ ) {
        // Scan in x, generating pixels
        if ( sglContext->pixelData == PixelContent::PATCH_POINTER ) {
            *patch++ = sglContext->currentPatch;
        } else {
            *pix++ = sglContext->currentPixel;
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
polyScanFlat(SGL_CONTEXT *sglContext, Polygon *p, Window *win)
{
    int i;
    int li;
    int ri;
    int y;
    int ly;
    int ry;
    int top;
    int rem;
    double yMin;
    PolygonVertex l{};
    PolygonVertex r{};
    PolygonVertex dl{};
    PolygonVertex dr{};

    yMin = HUGE;
    top = -1;
    for ( i = 0; i < p->n; i++ ) {
        // Find top vertex (y positions down)
        if ( p->vertices[i].sy < yMin ) {
            yMin = p->vertices[i].sy;
            top = i;
        }
    }

    li = ri = top; // Left and right vertex indices
    rem = p->n; // Number of vertices remaining
    y = ceil(yMin - 0.5); // Current scan line
    ly = ry = y - 1; // Lower end of left & right edges

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
            incrementalizeY((double *) &p->vertices[li], (double *) &p->vertices[i], (double *) &l, (double *) &dl, y);
            ly = floor(p->vertices[i].sy + .5);
            li = i;
        }
        while ( ry <= y && rem > 0 ) {
            // Advance right edge?
            rem--;
            i = ri + 1; // Step cw down right edge
            if ( i >= p->n ) {
                i = 0;
            }
            incrementalizeY((double *) &p->vertices[ri], (double *) &p->vertices[i], (double *) &r, (double *) &dr, y);
            ry = floor(p->vertices[i].sy + .5);
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
            increment((double *) &l, (double *) &dl);
            increment((double *) &r, (double *) &dr);
        }
    }
}
