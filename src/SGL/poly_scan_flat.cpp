/* Does no interpolations, for flat shading without Z buffering etc... PhB */

/*
 * Generic Convex Polygon Scan Conversion and Clipping
 * by Paul Heckbert
 * from "Graphics Gems", Academic Press, 1990
 */

/*
 * poly_scan.c: point-sampled scan conversion of convex polygons
 *
 * Paul Heckbert	1985, Dec 1989
 */

#include "common/mymath.h"
#include "SGL/poly.h"
#include "SGL/sgl.h"

/*
 * incrementalize_y: put intersection of line Y=y+.5 with edge between positions
 * p1 and p2 in p, put change with respect to y in dp
 */

static void incrementalize_y(double *p1, double *p2, double *p, double *dp, int y) {
    double dy, frac;

    dy = ((Poly_vert *) p2)->sy - ((Poly_vert *) p1)->sy;
    if ( dy == 0. ) {
        dy = 1.;
    }
    frac = y + .5 - ((Poly_vert *) p1)->sy;

    /* interpolate only sx */
    *dp = (*p2 - *p1) / dy;
    *p = *p1 + *dp * frac;
}

static void increment(double *p, double *dp) {
    /* interpolate only sx */
    *p += *dp;
}

/* scanline: output scanline by sampling polygon at Y=y+.5 */

static void scanline(int y, Poly_vert *l, Poly_vert *r, Window *win) {
    int x, lx, rx;
    SGL_PIXEL *pix;

    lx = ceil(l->sx - .5);
    if ( lx < win->x0 ) {
        lx = win->x0;
    }
    rx = floor(r->sx - .5);
    if ( rx > win->x1 ) {
        rx = win->x1;
    }
    if ( lx > rx ) {
        return;
    }

    pix = GLOBAL_sgl_currentContext->frameBuffer + y * GLOBAL_sgl_currentContext->width + lx;
    for ( x = lx; x <= rx; x++ ) {        /* scan in x, generating pixels */
        *pix++ = GLOBAL_sgl_currentContext->currentPixel;
    }
}

/*
 * poly_scan: Scan convert a polygon, calling pixelproc at each pixel with an
 * interpolated Poly_vert structure.  Polygon can be clockwise or ccw.
 * Polygon is clipped in 2-D to win, the screen space window.
 *
 * Scan conversion is done on the basis of Poly_vert fields sx and sy.
 * These two must always be interpolated, and only they have special meaning
 * to this code; any other fields are blindly interpolated regardless of
 * their semantics.
 *
 * The pixelproc subroutine takes the arguments:
 *
 *	pixelproc(x, y, point)
 *	int x, y;
 *	Poly_vert *point;
 *
 * All the fields of point indicated by p->mask will be valid inside pixelproc
 * except sx and sy.  If they were computed, they would have values
 * sx=x+.5 and sy=y+.5, since sampling is done at pixel centers.
 */

void poly_scan_flat(Poly *p, Window *win)
/* polygon */
/* 2-D screen space clipping window */
{
    int i, li, ri, y, ly, ry, top, rem;
    double ymin;
    Poly_vert l, r, dl, dr;

    ymin = HUGE;
    top = -1;
    for ( i = 0; i < p->n; i++ ) {        /* find top vertex (y positions down) */
        if ( p->vert[i].sy < ymin ) {
            ymin = p->vert[i].sy;
            top = i;
        }
    }

    li = ri = top;            /* left and right vertex indices */
    rem = p->n;                /* number of vertices remaining */
    y = ceil(ymin - .5);            /* current scan line */
    ly = ry = y - 1;            /* lower end of left & right edges */

    while ( rem > 0 ) {    /* scan in y, activating new edges on left & right */
        /* as scan line passes over new vertices */

        while ( ly <= y && rem > 0 ) {    /* advance left edge? */
            rem--;
            i = li - 1;            /* step ccw down left side */
            if ( i < 0 ) {
                i = p->n - 1;
            }
            incrementalize_y((double *) &p->vert[li], (double *) &p->vert[i], (double *) &l, (double *) &dl, y);
            ly = floor(p->vert[i].sy + .5);
            li = i;
        }
        while ( ry <= y && rem > 0 ) {    /* advance right edge? */
            rem--;
            i = ri + 1;            /* step cw down right edge */
            if ( i >= p->n ) {
                i = 0;
            }
            incrementalize_y((double *) &p->vert[ri], (double *) &p->vert[i], (double *) &r, (double *) &dr, y);
            ry = floor(p->vert[i].sy + .5);
            ri = i;
        }

        while ( y < ly && y < ry ) {        /* do scanlines till end of l or r edge */
            if ( y >= win->y0 && y <= win->y1 ) {
                if ( l.sx <= r.sx ) {
                    scanline(y, &l, &r, win);
                } else {
                    scanline(y, &r, &l, win);
                }
            }
            y++;
            increment((double *) &l, (double *) &dl);
            increment((double *) &r, (double *) &dr);
        }
    }
}

