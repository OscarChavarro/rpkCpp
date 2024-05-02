/**
Generic Convex Polygon Scan Conversion and Clipping by Paul Heckbert
from "Graphics Gems", Academic Press, 1990

Homogeneous 3-D convex polygon clipper Paul Heckbert 1985, Dec 1989

Note that original algorithm is available at
https://github.com/erich666/GraphicsGems/tree/master/gems/PolyScan
*/

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "SGL/poly.h"

static inline void
polygonSwap(Polygon *a, Polygon *b) {
    Polygon temporary = *a;
    *a = *b;
    *b = temporary;
}

/**
Clip convex polygon p against a plane,
copying the portion satisfying sign*s[index] < k*sw into q,
where s is a Poly_vert* cast as a double*.
index is an index into the array of doubles at each vertex, such that
s[index] is sx, sy, or sz (screen space x, y, or z).
Thus, to clip against xMin, use
polyClipToHalfSpace(p, q, X_INDEX, -1.0, -xMin);
and to clip against xMax, use
polyClipToHalfSpace(p, q, X_INDEX, 1.0,  xMax);
*/
static void
polyClipToHalfSpace(Polygon *p, Polygon *q, int index, double sign, double k) {
    const double *up;
    const double *vp;
    double *wp;
    double t;
    double tu;
    double tv;

    q->n = 0;
    q->mask = p->mask;

    // Start with u=vert[n-1], v=vert[0]
    PolygonVertex *u = &p->vertices[p->n - 1];
    tu = sign * u->getCoord(index) - u->sw * k;
    PolygonVertex *v = &p->vertices[0];
    for ( int i = p->n; i > 0; i-- ) {
        // On old polygon (p), u is previous vertex, v is current vertex
        // tv is negative if vertex v is in
        tv = sign * v->getCoord(index) - v->sw * k;
        if ( ((tu <= 0.0) && (tv > 0.0)) || ((tu > 0.0) && (tv <= 0.0)) ) {
            // Edge crosses plane; add intersection point to q
            t = tu / (tu - tv);
            up = (double *) u;
            vp = (double *) v;
            wp = (double *) &q->vertices[q->n];
            for ( unsigned long m = p->mask; m != 0; m >>= 1, up++, vp++, wp++ ) {
                if ( m & 1 ) {
                    *wp = *up + t * (*vp - *up);
                }
            }
            q->n++;
        }
        if ( tv <= 0.0 ) {
            // Vertex v is in, copy it to q
            q->vertices[q->n++] = *v;
        }
        u = v;
        tu = tv;
        v++;
    }
}

static inline void
polygonClipAndSwap(int elementIndex, double sign, double k, Polygon *p, Polygon *q, Polygon *p1) {
    polyClipToHalfSpace(p, q, elementIndex, sign, sign * k);
    if ( q->n == 0 ) {
        p1->n = 0;
    }
    polygonSwap(p, q);
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
int
polyClipToBox(Polygon *p1, const PolygonBox *box) {
    int x0out = 0;
    int x1out = 0;
    int y0out = 0;
    int y1out = 0;
    int z0out = 0;
    int z1out = 0;
    int i;
    const PolygonVertex *v;
    Polygon p2{};
    Polygon *p;
    Polygon *q;

    if ( p1->n + 6 > MAXIMUM_SIDES_PER_POLYGON ) {
        fprintf(stderr, "polyClipToBox: too many vertices: %d (max=%d-6)\n",
                p1->n, MAXIMUM_SIDES_PER_POLYGON);
        exit(1);
    }

    // Count vertices "outside" with respect to each of the six planes
    for ( v = p1->vertices, i = p1->n; i > 0; i--, v++ ) {
        if ( v->sx < box->x0 * v->sw ) {
            // Out on left
            x0out++;
        }
        if ( v->sx > box->x1 * v->sw ) {
            // Out on right
            x1out++;
        }
        if ( v->sy < box->y0 * v->sw ) {
            // Out on top
            y0out++;
        }
        if ( v->sy > box->y1 * v->sw ) {
            // Out on bottom
            y1out++;
        }
        if ( v->sz < box->z0 * v->sw ) {
            // Out on near
            z0out++;
        }
        if ( v->sz > box->z1 * v->sw ) {
            // Out on far
            z1out++;
        }
    }

    // Check if all vertices inside
    if ( x0out + x1out + y0out + y1out + z0out + z1out == 0 ) {
        return POLY_CLIP_IN;
    }

    // Check if all vertices are "outside" any of the six planes
    if ( x0out == p1->n || x1out == p1->n || y0out == p1->n ||
         y1out == p1->n || z0out == p1->n || z1out == p1->n ) {
        p1->n = 0;
        return POLY_CLIP_OUT;
    }

    // Now clip against each of the planes that might cut the polygon,
    // at each step toggling between polygons p1 and p2
    p = p1;
    q = &p2;
    if ( x0out ) {
        polygonClipAndSwap(0 /*sx*/, -1.0, box->x0, p, q, p1);
    }
    if ( x1out ) {
        polygonClipAndSwap(0 /*sx*/, 1.0, box->x1, p, q, p1);
    }
    if ( y0out ) {
        polygonClipAndSwap(1 /*sy*/, -1.0, box->y0, p, q, p1);
    }
    if ( y1out ) {
        polygonClipAndSwap(1 /*sy*/, 1.0, box->y1, p, q, p1);
    }
    if ( z0out ) {
        polygonClipAndSwap(2 /*sz*/, -1.0, box->z0, p, q, p1);
    }
    if ( z1out ) {
        polygonClipAndSwap(2 /*sz*/, 1.0, box->z1, p, q, p1);
    }

    // If result ended up in p2 then copy it to p1
    if ( p == &p2 ) {
        int n = (int)(sizeof(Polygon) - (MAXIMUM_SIDES_PER_POLYGON - p2.n) * sizeof(PolygonVertex));
        memcpy(p1, &p2, n);
    }
    return POLY_CLIP_PARTIAL;
}
