/* sgl.c: Small Graphics Library. Software rendering into a user
 * accessible memory buffer. E.g. for clustering where a small number
 * of patches needs to be ID rendered very often. */

#include <cstdlib>

#include "common/error.h"
#include "common/linealAlgebra/Float.h"
#include "SGL/poly.h"
#include "SGL/sgl.h"

Poly_vert *poly_dummy;        /* used superficially by POLY_MASK macro */

SGL_CONTEXT *current_sgl_context = (SGL_CONTEXT *)nullptr;

SGL_CONTEXT *sglOpen(int width, int height) {
    SGL_CONTEXT *context = (SGL_CONTEXT *)malloc(sizeof(SGL_CONTEXT));

    current_sgl_context = context;

    /* frame buffer */
    context->width = width;
    context->height = height;
    context->fbuf = (SGL_PIXEL *)malloc(width * height * sizeof(SGL_PIXEL));

    /* no Z buffer */
    context->zbuf = (SGL_ZVAL *) nullptr;

    /* transform stack and current transform */
    context->curtrans = context->transform_stack;
    *context->curtrans = IdentityTransform4x4;

    context->curpixel = 0;

    context->clipping = true;

    /* default viewport and depth range */
    context->vp_x = 0;
    context->vp_y = 0;
    context->vp_width = width;
    context->vp_height = height;
    context->near = 0.;
    context->far = 1.;

    return context;
}

void sglClose(SGL_CONTEXT *context) {
    free((char *) context->fbuf);
    if ( context->zbuf ) {
        free((char *) context->zbuf);
    }

    free((char *) context);

    if ( context == current_sgl_context ) {
        current_sgl_context = (SGL_CONTEXT *) nullptr;
    }
}

SGL_CONTEXT *sglMakeCurrent(SGL_CONTEXT *context) {
    SGL_CONTEXT *old_context = current_sgl_context;
    current_sgl_context = context;
    return old_context;
}

void sglClearFrameBuffer(SGL_PIXEL backgroundcol) {
    SGL_PIXEL *pix, *lpix;
    int i, j;

    lpix = current_sgl_context->fbuf + current_sgl_context->vp_y * current_sgl_context->width +
           current_sgl_context->vp_x;
    for ( j = 0; j < current_sgl_context->vp_height; j++, lpix += current_sgl_context->width ) {
        for ( pix = lpix, i = 0; i < current_sgl_context->vp_width; i++ ) {
            *pix++ = backgroundcol;
        }
    }
}

void sglClearZBuffer(SGL_ZVAL defzval) {
    SGL_ZVAL *zval, *lzval;
    int i, j;

    lzval = current_sgl_context->zbuf + current_sgl_context->vp_y * current_sgl_context->width +
            current_sgl_context->vp_x;
    for ( j = 0; j < current_sgl_context->vp_height; j++, lzval += current_sgl_context->width ) {
        for ( zval = lzval, i = 0; i < current_sgl_context->vp_width; i++ ) {
            *zval++ = defzval;
        }
    }
}

void sglClear(SGL_PIXEL backgroundcol, SGL_ZVAL defzval) {
    sglClearFrameBuffer(backgroundcol);
    sglClearZBuffer(defzval);
}

void sglDepthTesting(SGL_BOOLEAN on) {
    if ( on ) {
        if ( current_sgl_context->zbuf ) {
            return;
        } else {
            current_sgl_context->zbuf = (SGL_ZVAL *)malloc(
                    current_sgl_context->width * current_sgl_context->height * sizeof(SGL_ZVAL));
        }
    } else {
        if ( current_sgl_context->zbuf ) {
            free((char *) current_sgl_context->zbuf);
            current_sgl_context->zbuf = (SGL_ZVAL *) nullptr;
        } else {
            return;
        }
    }
}

void sglClipping(SGL_BOOLEAN on) {
    current_sgl_context->clipping = on;
}

void sglPushMatrix() {
    Matrix4x4 *oldtrans;

    if ( current_sgl_context->curtrans - current_sgl_context->transform_stack >= SGL_TRANSFORM_STACK_SIZE - 1 ) {
        logError("sglPushMatrix", "Matrix stack overflow");
        return;
    }

    oldtrans = current_sgl_context->curtrans;
    current_sgl_context->curtrans++;
    *current_sgl_context->curtrans = *oldtrans;
}

void sglPopMatrix() {
    if ( current_sgl_context->curtrans <= current_sgl_context->transform_stack ) {
        logError("sglPopMatrix", "Matrix stack underflow");
        return;
    }

    current_sgl_context->curtrans--;
}

void sglLoadMatrix(Matrix4x4 xf) {
    *current_sgl_context->curtrans = xf;
}

void sglMultMatrix(Matrix4x4 xf) {
    *current_sgl_context->curtrans = TransCompose(*current_sgl_context->curtrans, xf);
}

void sglSetColor(SGL_PIXEL col) {
    current_sgl_context->curpixel = col;
}

void sglViewport(int x, int y, int width, int height) {
    current_sgl_context->vp_x = x;
    current_sgl_context->vp_y = y;
    current_sgl_context->vp_width = width;
    current_sgl_context->vp_height = height;
}

void sglPolygon(int nrverts, Vector3D *verts) {
    Poly pol;
    Poly_vert *pv;
    Window win;
    Poly_box clip_box = {-1., 1., -1., 1., -1., 1.};
    int i;

    if ( nrverts > (current_sgl_context->clipping ? (POLY_NMAX - 6) : POLY_NMAX)) {
        logError("sglPolygon", "Too many vertices (max. %d)", POLY_NMAX);
        return;
    }

    /* transform the vertices and fill in a Poly */
    for ( i = 0, pv = &pol.vert[0]; i < nrverts; i++, pv++ ) {
        Vector4D v;
        v.x = verts[i].x;
        v.y = verts[i].y;
        v.z = verts[i].z;
        v.w = 1.;
        TRANSFORM_POINT_4D(*current_sgl_context->curtrans, v, v);
        if ( v.w > -EPSILON && v.w < EPSILON ) {
            return;
        }
        pv->sx = v.x;
        pv->sy = v.y;
        pv->sz = v.z;
        pv->sw = v.w;
    }
    pol.n = nrverts;
    pol.mask = 0;

    if ( current_sgl_context->clipping ) {
        pol.mask = POLY_MASK(sx) | POLY_MASK(sy) | POLY_MASK(sz) | POLY_MASK(sw);
        if ( poly_clip_to_box(&pol, &clip_box) == POLY_CLIP_OUT ) {
            return;
        }
    }

    /* perspective divide and transformation to viewport and depth range */
    for ( i = 0, pv = &pol.vert[0]; i < pol.n; i++, pv++ ) {
        pv->sx = (double) current_sgl_context->vp_x +
                 (pv->sx / pv->sw + 1.) * (double) current_sgl_context->vp_width * 0.5;
        pv->sy = (double) current_sgl_context->vp_y +
                 (pv->sy / pv->sw + 1.) * (double) current_sgl_context->vp_height * 0.5;
        pv->sz = (current_sgl_context->near + (pv->sz / pv->sw + 1.) * current_sgl_context->far * 0.5) *
                 (double) SGL_ZMAX;
    }

    /* window */
    win.x0 = current_sgl_context->vp_x;
    win.y0 = current_sgl_context->vp_y;
    win.x1 = current_sgl_context->vp_x + current_sgl_context->vp_width - 1;
    win.y1 = current_sgl_context->vp_y + current_sgl_context->vp_height - 1;

    /* scan convert the polygon: use optimized version for flat shading
     * with or without Z buffering. */
    if ( !current_sgl_context->zbuf ) {
        poly_scan_flat(&pol, &win);
    } else {
        poly_scan_z(&pol, &win);
    }
}
