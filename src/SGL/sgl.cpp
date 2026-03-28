/**
Small Graphics Library. Software rendering into a user
accessible memory buffer. E.g. for clustering where a small number
of patches needs to be ID rendered very often
*/
#include <cstddef>

#include "common/Error.h"

#include "SGL/poly.h"
#include "SGL/sgl.h"

static Matrix4x4 globalIdentityMatrix(
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
);

/**
Creates and destroys an SGL rendering context.
*/
SGL_CONTEXT::SGL_CONTEXT(int width, int height):
    transformStack(),
    elementBuffer(),
    currentElement()
{
    // Frame buffer
    this->width = width;
    this->height = height;
    frameBuffer = new SGL_PIXEL[width * height];
    patchBuffer = new Patch *[width * height];

    for ( int i = 0; i < width * height; i++ ) {
        patchBuffer[i] = nullptr;
    }

    pixelData = SglPixelContent::PIXEL;

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
}

static void
sglClearFrameBuffer(SGL_CONTEXT *sglContext, SGL_PIXEL backgroundColor) {
    const int viewportOrigin = sglContext->vp_y * sglContext->width + sglContext->vp_x;
    for ( int j = 0; j < sglContext->vp_height; j++ ) {
        const int rowStart = viewportOrigin + j * sglContext->width;
        for ( int i = 0; i < sglContext->vp_width; i++ ) {
            sglContext->frameBuffer[rowStart + i] = backgroundColor;
        }
    }
}

/**
Returns current sgl renderer
*/
void
SGL_CONTEXT::sglClearZBuffer(const SGL_Z_VALUE defZVal) const {
    const int viewportOrigin = vp_y * width + vp_x;
    for ( int j = 0; j < vp_height; j++ ) {
        const int rowStart = viewportOrigin + j * width;
        for ( int i = 0; i < vp_width; i++ ) {
            depthBuffer[rowStart + i] = defZVal;
        }
    }
}

void
SGL_CONTEXT::sglClear(SGL_PIXEL backgroundColor, SGL_Z_VALUE defZVal) {
    sglClearFrameBuffer(this, backgroundColor);
    sglClearZBuffer(defZVal);
}

void
SGL_CONTEXT::sglDepthTesting(bool on) {
    if ( on ) {
        if ( depthBuffer != nullptr ) {
            return;
        } else {
            depthBuffer = new SGL_Z_VALUE[width * height];
        }
    } else {
        if ( depthBuffer != nullptr ) {
            delete[] depthBuffer;
            depthBuffer = nullptr;
        } else {
            return;
        }
    }
}

void
SGL_CONTEXT::sglClipping(bool on) {
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
SGL_CONTEXT::sglSetPatch(const Patch *patch) {
    pixelData = SglPixelContent::PATCH_POINTER;
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
    Window win{};
    PolygonBox clip_box = {-1.0, 1.0, -1.0, 1.0, -1.0, 1.0};

    if ( numberOfVertices > (clipping ? (MAXIMUM_SIDES_PER_POLYGON - 6) : MAXIMUM_SIDES_PER_POLYGON) ) {
        Error::error("sglPolygon", "Too many vertices (max. %d)", MAXIMUM_SIDES_PER_POLYGON);
        return;
    }

    // Transform the vertices and fill in a Poly
    for ( int i = 0; i < numberOfVertices; i++ ) {
        Vector4D v{};
        PolygonVertex &vertex = pol.vertices[i];
        v.x = vertices[i].x;
        v.y = vertices[i].y;
        v.z = vertices[i].z;
        v.w = 1.0;
        currentTransform->transformPoint4D(v, v);
        if ( v.w > -Numeric::EPSILON && v.w < Numeric::EPSILON ) {
            return;
        }
        vertex.sx = v.x;
        vertex.sy = v.y;
        vertex.sz = v.z;
        vertex.sw = v.w;
    }
    pol.n = numberOfVertices;
    pol.mask = 0;

    if ( clipping ) {
        pol.mask = polyMask(offsetof(PolygonVertex, sx)) |
                   polyMask(offsetof(PolygonVertex, sy)) |
                   polyMask(offsetof(PolygonVertex, sz)) |
                   polyMask(offsetof(PolygonVertex, sw));
        if ( polyClipToBox(&pol, &clip_box) == POLY_CLIP_OUT ) {
            return;
        }
    }

    // Perspective divide and transformation to viewport and depth range
    for ( int i = 0; i < pol.n; i++ ) {
        PolygonVertex &vertex = pol.vertices[i];
        vertex.sx = static_cast<double>(vp_x) + (vertex.sx / vertex.sw + 1.0) * static_cast<double>(vp_width) * 0.5;
        vertex.sy = static_cast<double>(vp_y) + (vertex.sy / vertex.sw + 1.0) * static_cast<double>(vp_height) * 0.5;
        vertex.sz = (near + (vertex.sz / vertex.sw + 1.0) * far * 0.5) * static_cast<double>(SGL_MAXIMUM_Z);
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
