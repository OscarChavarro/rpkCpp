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

/**
Creates, destroys an SGL rendering context. sglOpen() also makes the new context
the current context
*/
SGL_CONTEXT *
sglOpen(int width, int height) {
    SGL_CONTEXT *context = (SGL_CONTEXT *)malloc(sizeof(SGL_CONTEXT));

    GLOBAL_sgl_currentContext = context;

    // Frame buffer
    context->width = width;
    context->height = height;
    context->frameBuffer = new SGL_PIXEL[width * height];
    context->patchBuffer = new Patch *[width * height];
    context->pixelData = PixelContent::PIXEL;

    // No Z buffer
    context->depthBuffer = nullptr;

    // Transform stack and current transform
    context->currentTransform = context->transformStack;
    *context->currentTransform = GLOBAL_matrix_identityTransform4x4;

    context->currentPixel = 0;
    context->currentPatch = nullptr;

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

    if ( context->frameBuffer != nullptr ) {
        delete []context->frameBuffer;
    }

    if ( context->patchBuffer != nullptr ) {
        delete []context->patchBuffer;
    }

    if ( context->depthBuffer != nullptr ) {
        free(context->depthBuffer);
    }

    if ( context == GLOBAL_sgl_currentContext ) {
        GLOBAL_sgl_currentContext = nullptr;
    }
    free(context);
}

/**
Makes the specified context current, returns the previous current context
*/
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
sglClearFrameBuffer(SGL_CONTEXT *sglContext, SGL_PIXEL backgroundColor) {
    SGL_PIXEL *pixel;
    SGL_PIXEL *lPixel;
    int i;

    lPixel = sglContext->frameBuffer +
            sglContext->vp_y * sglContext->width +
            sglContext->vp_x;
    for ( int j = 0; j < sglContext->vp_height; j++, lPixel += sglContext->width ) {
        for ( pixel = lPixel, i = 0; i < sglContext->vp_width; i++ ) {
            *pixel++ = backgroundColor;
        }
    }
}

/**
Returns current sgl renderer
*/
void
sglClearZBuffer(SGL_CONTEXT *sglContext, SGL_Z_VALUE defZVal) {
    SGL_Z_VALUE *zVal;
    SGL_Z_VALUE *lzVal;
    int i;
    int j;

    lzVal = sglContext->depthBuffer + sglContext->vp_y * sglContext->width +
            sglContext->vp_x;
    for ( j = 0; j < sglContext->vp_height; j++, lzVal += sglContext->width ) {
        for ( zVal = lzVal, i = 0; i < sglContext->vp_width; i++ ) {
            *zVal++ = defZVal;
        }
    }
}

void
sglClear(SGL_CONTEXT *sglContext, SGL_PIXEL backgroundColor, SGL_Z_VALUE defZVal) {
    sglClearFrameBuffer(sglContext, backgroundColor);
    sglClearZBuffer(sglContext, defZVal);
}

void
sglDepthTesting(SGL_CONTEXT *sglContext, SGL_BOOLEAN on) {
    if ( on ) {
        if ( sglContext->depthBuffer ) {
            return;
        } else {
            sglContext->depthBuffer = (SGL_Z_VALUE *)malloc(
            sglContext->width * sglContext->height * sizeof(SGL_Z_VALUE));
        }
    } else {
        if ( sglContext->depthBuffer ) {
            free(sglContext->depthBuffer);
            sglContext->depthBuffer = nullptr;
        } else {
            return;
        }
    }
}

void
sglClipping(SGL_CONTEXT *sglContext, SGL_BOOLEAN on) {
    sglContext->clipping = on;
}

void
sglLoadMatrix(SGL_CONTEXT *sglContext, Matrix4x4 xf) {
    *sglContext->currentTransform = xf;
}

void
sglMultiplyMatrix(SGL_CONTEXT *sglContext, Matrix4x4 xf) {
    *sglContext->currentTransform = transComposeMatrix(*sglContext->currentTransform, xf);
}

void
sglSetColor(SGL_CONTEXT *sglContext, SGL_PIXEL col) {
    sglContext->currentPixel = col;
}

void
sglSetPatch(SGL_CONTEXT *sglContext, Patch *patch) {
    sglContext->pixelData = PixelContent::PATCH_POINTER;
    sglContext->currentPatch = patch;
}

void
sglViewport(SGL_CONTEXT *sglContext, int x, int y, int width, int height) {
    sglContext->vp_x = x;
    sglContext->vp_y = y;
    sglContext->vp_width = width;
    sglContext->vp_height = height;
}

void
sglPolygon(SGL_CONTEXT *sglContext, int numberOfVertices, Vector3D *vertices) {
    Polygon pol{};
    PolygonVertex *pv;
    Window win{};
    PolygonBox clip_box = {-1.0, 1.0, -1.0, 1.0, -1.0, 1.0};
    int i;

    if ( numberOfVertices > (sglContext->clipping ? (MAXIMUM_SIDES_PER_POLYGON - 6) : MAXIMUM_SIDES_PER_POLYGON)) {
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
        transformPoint4D(*sglContext->currentTransform, v, v);
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

    if ( sglContext->clipping ) {
        pol.mask = POLY_MASK(sx) | POLY_MASK(sy) | POLY_MASK(sz) | POLY_MASK(sw);
        if ( polyClipToBox(&pol, &clip_box) == POLY_CLIP_OUT ) {
            return;
        }
    }

    // Perspective divide and transformation to viewport and depth range
    for ( i = 0, pv = &pol.vertices[0]; i < pol.n; i++, pv++ ) {
        pv->sx = (double)sglContext->vp_x +
                 (pv->sx / pv->sw + 1.0) * (double)sglContext->vp_width * 0.5;
        pv->sy = (double) sglContext->vp_y +
                 (pv->sy / pv->sw + 1.0) * (double)sglContext->vp_height * 0.5;
        pv->sz = (sglContext->near + (pv->sz / pv->sw + 1.) * sglContext->far * 0.5) *
                 (double) SGL_MAXIMUM_Z;
    }

    // Window
    win.x0 = sglContext->vp_x;
    win.y0 = sglContext->vp_y;
    win.x1 = sglContext->vp_x + sglContext->vp_width - 1;
    win.y1 = sglContext->vp_y + sglContext->vp_height - 1;

    // Scan convert the polygon: use optimized version for flat shading
    // with or without Z buffering
    if ( !sglContext->depthBuffer ) {
        polyScanFlat(sglContext, &pol, &win);
    } else {
        polyScanZ(sglContext, &pol, &win);
    }
}
