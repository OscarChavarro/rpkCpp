/**
Flat shading + Z buffering
Generic Convex Polygon Scan Conversion and Clipping
by Paul Heckbert
from "Graphics Gems", Academic Press, 1990
Point-sampled scan conversion of convex polygons

Paul Heckbert 1985, Dec 1989
*/

#include "common/mymath.h"
#include "SGL/poly.h"
#include "SGL/sgl.h"

/**
incrementalizeY: put intersection of line Y=y+.5 with edge between positions
p1 and p2 in p, put change with respect to y in dp
*/
static void
incrementalizeY(const double *p1, const double *p2, double *p, double *dp, int y) {
    double dy;
    double frac;

    dy = ((const PolygonVertex *) p2)->sy - ((const PolygonVertex *) p1)->sy;
    if ( dy == 0.0 ) {
        dy = 1.0;
    }
    frac = y + 0.5 - ((const PolygonVertex *) p1)->sy;

    // Interpolate only sx and sz (first and third field)
    dp[0] = (p2[0] - p1[0]) / dy;
    p[0] = p1[0] + dp[0] * frac;
    dp[2] = (p2[2] - p1[2]) / dy;
    p[2] = p1[2] + dp[2] * frac;
}

static void
increment(double *p, const double *dp) {
    // Increment only sx and sz
    p[0] += dp[0];
    p[2] += dp[2];
}

/**
Output scanline by sampling polygon at Y = y + 0.5
*/
static void
scanline(SGL_CONTEXT *sglContext, int y, const PolygonVertex *l, const PolygonVertex *r, const Window *win) {
    int dz;
    SGL_Z_VALUE *zValue;
    SGL_Z_VALUE z;
    double dx;
    double frac;
    double dzf;

    int lx = (int)std::ceil(l->sx - 0.5);
    if ( lx < win->x0 ) {
        lx = win->x0;
    }

    int rx = (int)std::floor(r->sx - 0.5);
    if ( rx > win->x1 ) {
        rx = win->x1;
    }
    if ( lx > rx ) {
        return;
    }

    dx = r->sx - l->sx;
    if ( dx == 0.0 ) {
        dx = 1.0;
    }
    frac = lx + 0.5 - l->sx;
    dzf = (r->sz - l->sz) / dx;
    z = (SGL_Z_VALUE) (l->sz + dzf * frac);
    dz = (int) dzf;

    int offset = y * sglContext->width + lx;
    SGL_PIXEL *pix = sglContext->frameBuffer + offset;
    Patch **patch = sglContext->patchBuffer + offset;

    zValue = sglContext->depthBuffer + offset;
    for ( int x = lx; x <= rx; x++ ) {
        // Scan in x, generating pixels
        if ( z <= *zValue ) {
            if ( sglContext->pixelData == PixelContent::PATCH_POINTER ) {
                *patch = sglContext->currentPatch;
            } else {
                *pix = sglContext->currentPixel;
            }
            *zValue = z;
        }
        pix++;
        patch++;
        zValue++;
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
except sx and sy.  If they were computed, they would have values
sx=x+.5 and sy=y+.5, since sampling is done at pixel centers.
*/
void
polyScanZ(SGL_CONTEXT *sglContext, Polygon *p,  const Window *window)
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
    y = (int)std::ceil(yMin - 0.5); // Current scan line
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
            ly = (int)std::floor(p->vertices[i].sy + 0.5);
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
            ry = (int)std::floor(p->vertices[i].sy + .5);
            ri = i;
        }

        while ( y < ly && y < ry ) {
            // Do scan lines till end of l or r edge
            if ( y >= window->y0 && y <= window->y1 ) {
                if ( l.sx <= r.sx ) {
                    scanline(sglContext, y, &l, &r, window);
                } else {
                    scanline(sglContext, y, &r, &l, window);
                }
            }
            y++;
            increment((double *) &l, (double *) &dl);
            increment((double *) &r, (double *) &dr);
        }
    }
}
