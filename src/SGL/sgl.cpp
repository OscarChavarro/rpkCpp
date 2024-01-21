/* sgl.c: Small Graphics Library. Software rendering into a user
 * accessible memory buffer. E.g. for clustering where a small number
 * of patches needs to be ID rendered very often. */

#include <cstdlib>

#include "common/error.h"
#include "common/linealAlgebra/Float.h"
#include "SGL/poly.h"
#include "SGL/sgl.h"

Poly_vert *GLOBAL_sgl_polyDummy;        /* used superficially by POLY_MASK macro */

SGL_CONTEXT *GLOBAL_sgl_currentContext = (SGL_CONTEXT *)nullptr;

SGL_CONTEXT *sglOpen(int width, int height) {
    SGL_CONTEXT *context = (SGL_CONTEXT *)malloc(sizeof(SGL_CONTEXT));

    GLOBAL_sgl_currentContext = context;

    /* frame buffer */
    context->width = width;
    context->height = height;
    context->frameBuffer = (SGL_PIXEL *)malloc(width * height * sizeof(SGL_PIXEL));

    /* no Z buffer */
    context->depthBuffer = (SGL_Z_VALUE *) nullptr;

    /* transform stack and current transform */
    context->currentTransform = context->transformStack;
    *context->currentTransform = GLOBAL_matrix_identityTransform4x4;

    context->currentPixel = 0;

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
    free((char *) context->frameBuffer);
    if ( context->depthBuffer ) {
        free((char *) context->depthBuffer);
    }

    free((char *) context);

    if ( context == GLOBAL_sgl_currentContext ) {
        GLOBAL_sgl_currentContext = (SGL_CONTEXT *) nullptr;
    }
}

SGL_CONTEXT *sglMakeCurrent(SGL_CONTEXT *context) {
    SGL_CONTEXT *old_context = GLOBAL_sgl_currentContext;
    GLOBAL_sgl_currentContext = context;
    return old_context;
}

/**
All the following operate on the current SGL context and behave very similar as
the corresponding functions in OpenGL
*/
static void
sglClearFrameBuffer(SGL_PIXEL backgroundColor) {
    SGL_PIXEL *pixel;
    SGL_PIXEL *lPixel;
    int i;
    int j;

    lPixel = GLOBAL_sgl_currentContext->frameBuffer + GLOBAL_sgl_currentContext->vp_y * GLOBAL_sgl_currentContext->width +
             GLOBAL_sgl_currentContext->vp_x;
    for ( j = 0; j < GLOBAL_sgl_currentContext->vp_height; j++, lPixel += GLOBAL_sgl_currentContext->width ) {
        for ( pixel = lPixel, i = 0; i < GLOBAL_sgl_currentContext->vp_width; i++ ) {
            *pixel++ = backgroundColor;
        }
    }
}

void sglClearZBuffer(SGL_Z_VALUE defzval) {
    SGL_Z_VALUE *zval, *lzval;
    int i, j;

    lzval = GLOBAL_sgl_currentContext->depthBuffer + GLOBAL_sgl_currentContext->vp_y * GLOBAL_sgl_currentContext->width +
            GLOBAL_sgl_currentContext->vp_x;
    for ( j = 0; j < GLOBAL_sgl_currentContext->vp_height; j++, lzval += GLOBAL_sgl_currentContext->width ) {
        for ( zval = lzval, i = 0; i < GLOBAL_sgl_currentContext->vp_width; i++ ) {
            *zval++ = defzval;
        }
    }
}

void sglClear(SGL_PIXEL backgroundcol, SGL_Z_VALUE defzval) {
    sglClearFrameBuffer(backgroundcol);
    sglClearZBuffer(defzval);
}

void sglDepthTesting(SGL_BOOLEAN on) {
    if ( on ) {
        if ( GLOBAL_sgl_currentContext->depthBuffer ) {
            return;
        } else {
            GLOBAL_sgl_currentContext->depthBuffer = (SGL_Z_VALUE *)malloc(
                    GLOBAL_sgl_currentContext->width * GLOBAL_sgl_currentContext->height * sizeof(SGL_Z_VALUE));
        }
    } else {
        if ( GLOBAL_sgl_currentContext->depthBuffer ) {
            free((char *) GLOBAL_sgl_currentContext->depthBuffer);
            GLOBAL_sgl_currentContext->depthBuffer = (SGL_Z_VALUE *) nullptr;
        } else {
            return;
        }
    }
}

void sglClipping(SGL_BOOLEAN on) {
    GLOBAL_sgl_currentContext->clipping = on;
}

void sglPushMatrix() {
    Matrix4x4 *oldtrans;

    if ( GLOBAL_sgl_currentContext->currentTransform - GLOBAL_sgl_currentContext->transformStack >= SGL_TRANSFORM_STACK_SIZE - 1 ) {
        logError("sglPushMatrix", "Matrix stack overflow");
        return;
    }

    oldtrans = GLOBAL_sgl_currentContext->currentTransform;
    GLOBAL_sgl_currentContext->currentTransform++;
    *GLOBAL_sgl_currentContext->currentTransform = *oldtrans;
}

void sglPopMatrix() {
    if ( GLOBAL_sgl_currentContext->currentTransform <= GLOBAL_sgl_currentContext->transformStack ) {
        logError("sglPopMatrix", "Matrix stack underflow");
        return;
    }

    GLOBAL_sgl_currentContext->currentTransform--;
}

void sglLoadMatrix(Matrix4x4 xf) {
    *GLOBAL_sgl_currentContext->currentTransform = xf;
}

void sglMultMatrix(Matrix4x4 xf) {
    *GLOBAL_sgl_currentContext->currentTransform = transComposeMatrix(*GLOBAL_sgl_currentContext->currentTransform, xf);
}

void sglSetColor(SGL_PIXEL col) {
    GLOBAL_sgl_currentContext->currentPixel = col;
}

void sglViewport(int x, int y, int width, int height) {
    GLOBAL_sgl_currentContext->vp_x = x;
    GLOBAL_sgl_currentContext->vp_y = y;
    GLOBAL_sgl_currentContext->vp_width = width;
    GLOBAL_sgl_currentContext->vp_height = height;
}

void sglPolygon(int nrverts, Vector3D *verts) {
    Poly pol;
    Poly_vert *pv;
    Window win;
    Poly_box clip_box = {-1., 1., -1., 1., -1., 1.};
    int i;

    if ( nrverts > (GLOBAL_sgl_currentContext->clipping ? (POLY_NMAX - 6) : POLY_NMAX)) {
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
        transformPoint4D(*GLOBAL_sgl_currentContext->currentTransform, v, v);
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

    if ( GLOBAL_sgl_currentContext->clipping ) {
        pol.mask = POLY_MASK(sx) | POLY_MASK(sy) | POLY_MASK(sz) | POLY_MASK(sw);
        if ( polyClipToBox(&pol, &clip_box) == POLY_CLIP_OUT ) {
            return;
        }
    }

    /* perspective divide and transformation to viewport and depth range */
    for ( i = 0, pv = &pol.vert[0]; i < pol.n; i++, pv++ ) {
        pv->sx = (double) GLOBAL_sgl_currentContext->vp_x +
                 (pv->sx / pv->sw + 1.) * (double) GLOBAL_sgl_currentContext->vp_width * 0.5;
        pv->sy = (double) GLOBAL_sgl_currentContext->vp_y +
                 (pv->sy / pv->sw + 1.) * (double) GLOBAL_sgl_currentContext->vp_height * 0.5;
        pv->sz = (GLOBAL_sgl_currentContext->near + (pv->sz / pv->sw + 1.) * GLOBAL_sgl_currentContext->far * 0.5) *
                 (double) SGL_MAXIMUM_Z;
    }

    /* window */
    win.x0 = GLOBAL_sgl_currentContext->vp_x;
    win.y0 = GLOBAL_sgl_currentContext->vp_y;
    win.x1 = GLOBAL_sgl_currentContext->vp_x + GLOBAL_sgl_currentContext->vp_width - 1;
    win.y1 = GLOBAL_sgl_currentContext->vp_y + GLOBAL_sgl_currentContext->vp_height - 1;

    /* scan convert the polygon: use optimized version for flat shading
     * with or without Z buffering. */
    if ( !GLOBAL_sgl_currentContext->depthBuffer ) {
        polyScanFlat(&pol, &win);
    } else {
        polyScanZ(&pol, &win);
    }
}
