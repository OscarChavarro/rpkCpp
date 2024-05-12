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

// Note this can change between drawing the main frame buffer, drawing Z-based form factors, etc.
SGL_CONTEXT *GLOBAL_sgl_currentContext{};

static Matrix4x4 globalIdentityMatrix = {
    {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    }
};

/**
Makes the specified context current, returns the previous current context
*/
SGL_CONTEXT *
sglMakeCurrent(SGL_CONTEXT *context) {
    SGL_CONTEXT *oldContext = GLOBAL_sgl_currentContext;
    GLOBAL_sgl_currentContext = context;
    return oldContext;
}

/**
Creates, destroys an SGL rendering context. sglOpen() also makes the new context
the current context
*/
SGL_CONTEXT::SGL_CONTEXT(int width, int height):
    transformStack(),
    elementBuffer(),
    currentElement()
{
    GLOBAL_sgl_currentContext = this;

    // Frame buffer
    this->width = width;
    this->height = height;
    frameBuffer = new SGL_PIXEL[width * height];
    patchBuffer = new Patch *[width * height];

    for ( int i = 0; i < width * height; i++ ) {
        patchBuffer[i] = nullptr;
    }

    pixelData = PixelContent::PIXEL;

    // No Z buffer
    depthBuffer = nullptr;

    // Transform stack and current transform
    currentTransform = transformStack;
    *currentTransform = globalIdentityMatrix;

    currentPixel = 0;
    currentPatch = nullptr;

    clipping = true;

    // Default viewport and depth range
    vp_x = 0;
    vp_y = 0;
    vp_width = width;
    vp_height = height;
    near = 0.0;
    far = 1.0;
}

SGL_CONTEXT::~SGL_CONTEXT() {
    if ( frameBuffer != nullptr ) {
        delete []frameBuffer;
    }

    if ( patchBuffer != nullptr ) {
        delete []patchBuffer;
    }

    if ( depthBuffer != nullptr ) {
        delete[] depthBuffer;
    }

    if ( this == GLOBAL_sgl_currentContext ) {
        GLOBAL_sgl_currentContext = nullptr;
    }
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
SGL_CONTEXT::sglClearZBuffer(const SGL_Z_VALUE defZVal) const {
    SGL_Z_VALUE *lzVal = depthBuffer + vp_y * width + vp_x;

    for ( int j = 0; j < vp_height; j++, lzVal += width ) {
        SGL_Z_VALUE *zVal = lzVal;
        for ( int i = 0; i < vp_width; i++ ) {
            *zVal++ = defZVal;
        }
    }
}

void
SGL_CONTEXT::sglClear(SGL_PIXEL backgroundColor, SGL_Z_VALUE defZVal) {
    sglClearFrameBuffer(this, backgroundColor);
    sglClearZBuffer(defZVal);
}

void
SGL_CONTEXT::sglDepthTesting(SGL_BOOLEAN on) {
    if ( on ) {
        if ( depthBuffer ) {
            return;
        } else {
            depthBuffer = new SGL_Z_VALUE[width * height];
        }
    } else {
        if ( depthBuffer ) {
            free(depthBuffer);
            depthBuffer = nullptr;
        } else {
            return;
        }
    }
}

void
SGL_CONTEXT::sglClipping(SGL_BOOLEAN on) {
    clipping = on;
}

void
SGL_CONTEXT::sglLoadMatrix(const Matrix4x4 *xf) const {
    *currentTransform = *xf;
}

void
SGL_CONTEXT::sglMultiplyMatrix(const Matrix4x4 *xf) const {
    *currentTransform = Matrix4x4::createTransComposeMatrix(currentTransform, xf);
}

void
SGL_CONTEXT::sglSetColor(SGL_PIXEL col) {
    currentPixel = col;
}

void
SGL_CONTEXT::sglSetPatch(Patch *patch) {
    pixelData = PixelContent::PATCH_POINTER;
    currentPatch = patch;
}

void
SGL_CONTEXT::sglViewport(int x, int y, int viewPortWidth, int viewPortHeight) {
    vp_x = x;
    vp_y = y;
    vp_width = viewPortWidth;
    vp_height = viewPortHeight;
}

void
SGL_CONTEXT::sglPolygon(const int numberOfVertices, const Vector3D *vertices) {
    Polygon pol{};
    PolygonVertex *pv;
    Window win{};
    PolygonBox clip_box = {-1.0, 1.0, -1.0, 1.0, -1.0, 1.0};
    int i;

    if ( numberOfVertices > (clipping ? (MAXIMUM_SIDES_PER_POLYGON - 6) : MAXIMUM_SIDES_PER_POLYGON) ) {
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
        currentTransform->transformPoint4D(v, v);
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

    if ( clipping ) {
        pol.mask = POLY_MASK(sx) | POLY_MASK(sy) | POLY_MASK(sz) | POLY_MASK(sw);
        if ( polyClipToBox(&pol, &clip_box) == POLY_CLIP_OUT ) {
            return;
        }
    }

    // Perspective divide and transformation to viewport and depth range
    for ( i = 0, pv = &pol.vertices[0]; i < pol.n; i++, pv++ ) {
        pv->sx = (double)vp_x + (pv->sx / pv->sw + 1.0) * (double)vp_width * 0.5;
        pv->sy = (double) vp_y + (pv->sy / pv->sw + 1.0) * (double)vp_height * 0.5;
        pv->sz = (near + (pv->sz / pv->sw + 1.0) * far * 0.5) * (double) SGL_MAXIMUM_Z;
    }

    // Window
    win.x0 = vp_x;
    win.y0 = vp_y;
    win.x1 = vp_x + vp_width - 1;
    win.y1 = vp_y + vp_height - 1;

    // Scan convert the polygon: use optimized version for flat shading with or without Z buffering
    if ( depthBuffer != nullptr ) {
        polyScanZ(this, &pol, &win);
    } else {
        polyScanFlat(this, &pol, &win);
    }
}
