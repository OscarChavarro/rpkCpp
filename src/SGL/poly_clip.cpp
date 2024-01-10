/**
Generic Convex Polygon Scan Conversion and Clipping by Paul Heckbert
from "Graphics Gems", Academic Press, 1990

Homogeneous 3-D convex polygon clipper
Paul Heckbert 1985, Dec 1989
*/

#include <cstdio>
#include <cstdlib>
#include <strings.h>

#include "SGL/poly.h"

#define SWAP(a, b, temp) {temp = a; a = b; b = temp;}
#define COORD(vert, i) ((double *)(vert))[i]

// Note: double check original ugly macro was correctly migrated to function, and
// it is working for the swaps...
/*
#define CLIP_AND_SWAP(elem, sign, k, p, q, r) { \
    poly_clip_to_halfspace(p, q, &v->elem-(double *)v, sign, sign*k); \
    if ( q->n == 0) { \
        p1->n = 0; return POLY_CLIP_OUT; \
    } \
    SWAP(p, q, r); \
}
*/

inline bool
CLIP_AND_SWAP(Poly *p1, Poly_vert *v, double *vElem, double sign, double k, Poly *p, Poly *q, Poly *r) {
    polyClipToHalfSpace(p, q, vElem-(double *)v, sign, sign*k);
    if ( q->n == 0) {
        p1->n = 0;
        return true;
    }
    SWAP(p, q, r);
    return false;
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
(n+6)gon, so POLY_NMAX in poly.h must be big enough to allow that.
*/
int
polyClipToBox(Poly *p1, Poly_box *box) {
    int x0out = 0, x1out = 0, y0out = 0, y1out = 0, z0out = 0, z1out = 0;
    int i;
    Poly_vert *v;
    Poly p2{};
    Poly *p;
    Poly *q;
    Poly *r;

    if ( p1->n + 6 > POLY_NMAX ) {
        fprintf(stderr, "polyClipToBox: too many vertices: %d (max=%d-6)\n",
                p1->n, POLY_NMAX);
        exit(1);
    }
    if ( sizeof(Poly_vert) / sizeof(double) > 32 ) {
        fprintf(stderr, "Poly_vert structure too big; must be <=32 doubles\n");
        exit(1);
    }

    // Count vertices "outside" with respect to each of the six planes
    for ( v = p1->vert, i = p1->n; i > 0; i--, v++ ) {
        if ( v->sx < box->x0 * v->sw ) {
            x0out++;
        }    /* out on left */
        if ( v->sx > box->x1 * v->sw ) {
            x1out++;
        }    /* out on right */
        if ( v->sy < box->y0 * v->sw )
            y0out++;    /* out on top */
        if ( v->sy > box->y1 * v->sw )
            y1out++;    /* out on bottom */
        if ( v->sz < box->z0 * v->sw )
            z0out++;    /* out on near */
        if ( v->sz > box->z1 * v->sw )
            z1out++;    /* out on far */
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
        if ( CLIP_AND_SWAP(p1, v, &v->sx, -1.0, box->x0, p, q, r) ) {
            return POLY_CLIP_OUT;
        }
    }
    if ( x1out ) {
        if ( CLIP_AND_SWAP(p1, v, &v->sx, 1.0, box->x1, p, q, r) ) {
            return POLY_CLIP_OUT;
        }
    }
    if ( y0out ) {
        if ( CLIP_AND_SWAP(p1, v, &v->sy, -1.0, box->y0, p, q, r) ) {
            return POLY_CLIP_OUT;
        }
    }
    if ( y1out ) {
        if ( CLIP_AND_SWAP(p1, v, &v->sy, 1.0, box->y1, p, q, r) ) {
            return POLY_CLIP_OUT;
        }
    }
    if ( z0out ) {
        if ( CLIP_AND_SWAP(p1, v, &v->sz, -1.0, box->z0, p, q, r) ) {
            return POLY_CLIP_OUT;
        }
    }
    if ( z1out ) {
        if ( CLIP_AND_SWAP(p1, v, &v->sz, 1.0, box->z1, p, q, r) ) {
            return POLY_CLIP_OUT;
        }
    }

    // If result ended up in p2 then copy it to p1
    if ( p == &p2 ) {
        bcopy(&p2, p1, sizeof(Poly) - (POLY_NMAX - p2.n) * sizeof(Poly_vert));
    }
    return POLY_CLIP_PARTIAL;
}

/**
Clip convex polygon p against a plane,
copying the portion satisfying sign*s[index] < k*sw into q,
where s is a Poly_vert* cast as a double*.
index is an index into the array of doubles at each vertex, such that
s[index] is sx, sy, or sz (screen space x, y, or z).
Thus, to clip against xmin, use
polyClipToHalfspace(p, q, XINDEX, -1., -xmin);
and to clip against xmax, use
polyClipToHalfspace(p, q, XINDEX,  1.,  xmax);
*/
void
polyClipToHalfSpace(Poly *p, Poly *q, int index, double sign, double k) {
    unsigned long m;
    double *up, *vp, *wp;
    Poly_vert *v;
    int i;
    Poly_vert *u;
    double t, tu, tv;

    q->n = 0;
    q->mask = p->mask;

    // Start with u=vert[n-1], v=vert[0]
    u = &p->vert[p->n - 1];
    tu = sign * COORD(u, index) - u->sw * k;
    for ( v = &p->vert[0], i = p->n; i > 0; i--, u = v, tu = tv, v++ ) {
        // On old polygon (p), u is previous vertex, v is current vertex
        // tv is negative if vertex v is in
        tv = sign * COORD(v, index) - v->sw * k;
        if (((tu <= 0.) ^ (tv <= 0.))) {
            // Edge crosses plane; add intersection point to q
            t = tu / (tu - tv);
            up = (double *) u;
            vp = (double *) v;
            wp = (double *) &q->vert[q->n];
            for ( m = p->mask; m != 0; m >>= 1, up++, vp++, wp++ ) {
                if ( m & 1 ) {
                    *wp = *up + t * (*vp - *up);
                }
            }
            q->n++;
        }
        if ( tv <= 0.0 ) {
            // Vertex v is in, copy it to q
            q->vert[q->n++] = *v;
        }
    }
}
