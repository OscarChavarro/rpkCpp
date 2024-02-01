/**
Small Graphics Library. Software rendering into a user
accessible memory buffer. E.g. for clustering where a small number
of patches needs to be ID rendered very often
*/

#include "common/error.h"
#include "common/linealAlgebra/Float.h"
#include "SGL/poly.h"
#include "SGL/sgl.h"

// Used superficially by POLY_MASK macro
PolygonVertex *GLOBAL_sgl_polyDummy;

SGL_CONTEXT *GLOBAL_sgl_currentContext{};

SGL_CONTEXT *
sglOpen(int width, int height) {
    SGL_CONTEXT *context = (SGL_CONTEXT *)malloc(sizeof(SGL_CONTEXT));

    GLOBAL_sgl_currentContext = context;

    // Frame buffer
    context->width = width;
    context->height = height;
    context->frameBuffer = (SGL_PIXEL *)malloc(width * height * sizeof(SGL_PIXEL));

    // No Z buffer
    context->depthBuffer = nullptr;

    // Transform stack and current transform
    context->currentTransform = context->transformStack;
    *context->currentTransform = GLOBAL_matrix_identityTransform4x4;

    context->currentPixel = 0;

    context->clipping = true;

    // Default viewport and depth range
    context->vp_x = 0;
    context->vp_y = 0;
    context->vp_width = width;
    context->vp_height = height;
    context->near = 0.0;
    context->far = 1.0;

    return context;
}

void
sglClose(SGL_CONTEXT *context) {
    if ( context == nullptr ) {
        return;
    }
    free(context->frameBuffer);
    if ( context->depthBuffer ) {
        free(context->depthBuffer);
    }

    if ( context == GLOBAL_sgl_currentContext ) {
        GLOBAL_sgl_currentContext = nullptr;
    }
    free(context);
}

SGL_CONTEXT *
sglMakeCurrent(SGL_CONTEXT *context) {
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

void
sglClearZBuffer(SGL_Z_VALUE defZVal) {
    SGL_Z_VALUE *zVal;
    SGL_Z_VALUE *lzVal;
    int i;
    int j;

    lzVal = GLOBAL_sgl_currentContext->depthBuffer + GLOBAL_sgl_currentContext->vp_y * GLOBAL_sgl_currentContext->width +
            GLOBAL_sgl_currentContext->vp_x;
    for ( j = 0; j < GLOBAL_sgl_currentContext->vp_height; j++, lzVal += GLOBAL_sgl_currentContext->width ) {
        for ( zVal = lzVal, i = 0; i < GLOBAL_sgl_currentContext->vp_width; i++ ) {
            *zVal++ = defZVal;
        }
    }
}

void
sglClear(SGL_PIXEL backgroundColor, SGL_Z_VALUE defZVal) {
    sglClearFrameBuffer(backgroundColor);
    sglClearZBuffer(defZVal);
}

void
sglDepthTesting(SGL_BOOLEAN on) {
    if ( on ) {
        if ( GLOBAL_sgl_currentContext->depthBuffer ) {
            return;
        } else {
            GLOBAL_sgl_currentContext->depthBuffer = (SGL_Z_VALUE *)malloc(
                    GLOBAL_sgl_currentContext->width * GLOBAL_sgl_currentContext->height * sizeof(SGL_Z_VALUE));
        }
    } else {
        if ( GLOBAL_sgl_currentContext->depthBuffer ) {
            free(GLOBAL_sgl_currentContext->depthBuffer);
            GLOBAL_sgl_currentContext->depthBuffer = (SGL_Z_VALUE *) nullptr;
        } else {
            return;
        }
    }
}

void
sglClipping(SGL_BOOLEAN on) {
    GLOBAL_sgl_currentContext->clipping = on;
}

void
sglLoadMatrix(Matrix4x4 xf) {
    *GLOBAL_sgl_currentContext->currentTransform = xf;
}

void
sglMultiplyMatrix(Matrix4x4 xf) {
    *GLOBAL_sgl_currentContext->currentTransform = transComposeMatrix(*GLOBAL_sgl_currentContext->currentTransform, xf);
}

void
sglSetColor(SGL_PIXEL col) {
    GLOBAL_sgl_currentContext->currentPixel = col;
}

void
sglViewport(int x, int y, int width, int height) {
    GLOBAL_sgl_currentContext->vp_x = x;
    GLOBAL_sgl_currentContext->vp_y = y;
    GLOBAL_sgl_currentContext->vp_width = width;
    GLOBAL_sgl_currentContext->vp_height = height;
}

void
sglPolygon(int numberOfVertices, Vector3D *vertices) {
    Polygon pol{};
    PolygonVertex *pv;
    Window win{};
    PolygonBox clip_box = {-1.0, 1.0, -1.0, 1.0, -1.0, 1.0};
    int i;

    if ( numberOfVertices > (GLOBAL_sgl_currentContext->clipping ? (MAXIMUM_SIDES_PER_POLYGON - 6) : MAXIMUM_SIDES_PER_POLYGON)) {
        logError("sglPolygon", "Too many vertices (max. %d)", MAXIMUM_SIDES_PER_POLYGON);
        return;
    }

    // Transform the vertices and fill in a Poly
    for ( i = 0, pv = &pol.vertices[0]; i < numberOfVertices; i++, pv++ ) {
        Vector4D v{};
        v.x = vertices[i].x;
        v.y = vertices[i].y;
        v.z = vertices[i].z;
        v.w = 1.0;
        transformPoint4D(*GLOBAL_sgl_currentContext->currentTransform, v, v);
        if ( v.w > -EPSILON && v.w < EPSILON ) {
            return;
        }
        pv->sx = v.x;
        pv->sy = v.y;
        pv->sz = v.z;
        pv->sw = v.w;
    }
    pol.n = numberOfVertices;
    pol.mask = 0;

    if ( GLOBAL_sgl_currentContext->clipping ) {
        pol.mask = POLY_MASK(sx) | POLY_MASK(sy) | POLY_MASK(sz) | POLY_MASK(sw);
        if ( polyClipToBox(&pol, &clip_box) == POLY_CLIP_OUT ) {
            return;
        }
    }

    // Perspective divide and transformation to viewport and depth range
    for ( i = 0, pv = &pol.vertices[0]; i < pol.n; i++, pv++ ) {
        pv->sx = (double) GLOBAL_sgl_currentContext->vp_x +
                 (pv->sx / pv->sw + 1.) * (double) GLOBAL_sgl_currentContext->vp_width * 0.5;
        pv->sy = (double) GLOBAL_sgl_currentContext->vp_y +
                 (pv->sy / pv->sw + 1.) * (double) GLOBAL_sgl_currentContext->vp_height * 0.5;
        pv->sz = (GLOBAL_sgl_currentContext->near + (pv->sz / pv->sw + 1.) * GLOBAL_sgl_currentContext->far * 0.5) *
                 (double) SGL_MAXIMUM_Z;
    }

    // Window
    win.x0 = GLOBAL_sgl_currentContext->vp_x;
    win.y0 = GLOBAL_sgl_currentContext->vp_y;
    win.x1 = GLOBAL_sgl_currentContext->vp_x + GLOBAL_sgl_currentContext->vp_width - 1;
    win.y1 = GLOBAL_sgl_currentContext->vp_y + GLOBAL_sgl_currentContext->vp_height - 1;

    // Scan convert the polygon: use optimized version for flat shading
    // with or without Z buffering
    if ( !GLOBAL_sgl_currentContext->depthBuffer ) {
        polyScanFlat(&pol, &win);
    } else {
        polyScanZ(&pol, &win);
    }
}
