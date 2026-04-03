/**
Small Graphics Library. Software rendering into a user
accessible memory buffer. E.g. for clustering where a small number
of patches needs to be ID rendered very often
*/
#include <cstddef>

#include "common/Error.h"
#include "render/sgl/Poly.h"
#include "render/sgl/SglContext.h"

const Matrix4x4 &
SglContext::identityMatrix() {
    static const Matrix4x4 identity(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
    return identity;
}

/**
Creates and destroys an SGL rendering context.
*/
SglContext::SglContext(int width, int height):
    transformStack(),
    galerkinElementBuffer(),
    currentGalerkinElement()
{
    // Frame buffer
    this->width = width;
    this->height = height;
    frameBuffer = new SGL_PIXEL[width * height];
    patchBuffer = new Patch *[width * height];
    galerkinElementBuffer = new Element *[width * height];

    for ( int i = 0; i < width * height; i++ ) {
        frameBuffer[i] = 0;
        patchBuffer[i] = nullptr;
        galerkinElementBuffer[i] = nullptr;
    }

    pixelData = SglPixelContent::PIXEL;

    // No Z buffer
    depthBuffer = nullptr;

    // Transform stack and current transform
    currentTransform = transformStack;
    *currentTransform = identityMatrix();

    currentPixel = 0;
    currentPatch = nullptr;
    currentGalerkinElement = nullptr;

    clipping = true;

    // Default viewport and depth range
    vp_x = 0;
    vp_y = 0;
    vp_width = width;
    vp_height = height;
    near = 0.0;
    far = 1.0;
}

SglContext::~SglContext() {
    if ( frameBuffer != nullptr ) {
        delete []frameBuffer;
    }

    if ( patchBuffer != nullptr ) {
        delete []patchBuffer;
    }

    if ( galerkinElementBuffer != nullptr ) {
        delete []galerkinElementBuffer;
    }

    if ( depthBuffer != nullptr ) {
        delete[] depthBuffer;
    }
}

void
SglContext::clearFrameBuffer(SglContext *sglContext, SGL_PIXEL backgroundColor) {
    const int viewportOrigin = sglContext->vp_y * sglContext->width + sglContext->vp_x;
    for ( int j = 0; j < sglContext->vp_height; j++ ) {
        const int rowStart = viewportOrigin + j * sglContext->width;
        for ( int i = 0; i < sglContext->vp_width; i++ ) {
            const int pixelIndex = rowStart + i;
            sglContext->frameBuffer[pixelIndex] = backgroundColor;
            sglContext->patchBuffer[pixelIndex] = nullptr;
            sglContext->galerkinElementBuffer[pixelIndex] = nullptr;
        }
    }
}

/**
Returns current sgl renderer
*/
void
SglContext::sglClearZBuffer(const SGL_Z_VALUE defZVal) const {
    const int viewportOrigin = vp_y * width + vp_x;
    for ( int j = 0; j < vp_height; j++ ) {
        const int rowStart = viewportOrigin + j * width;
        for ( int i = 0; i < vp_width; i++ ) {
            depthBuffer[rowStart + i] = defZVal;
        }
    }
}

void
SglContext::sglClear(SGL_PIXEL backgroundColor, SGL_Z_VALUE defZVal) {
    SglContext::clearFrameBuffer(this, backgroundColor);
    sglClearZBuffer(defZVal);
}

void
SglContext::sglDepthTesting(bool on) {
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
SglContext::sglClipping(bool on) {
    clipping = on;
}

void
SglContext::sglLoadMatrix(const Matrix4x4 *xf) const {
    *currentTransform = *xf;
}

void
SglContext::sglMultiplyMatrix(const Matrix4x4 *xf) const {
    *currentTransform = Matrix4x4::createTransComposeMatrix(currentTransform, xf);
}

void
SglContext::sglSetPatch(const Patch *patch) {
    pixelData = SglPixelContent::PATCH_POINTER;
    currentPatch = patch;
}

void
SglContext::sglSetGalerkinElement(const Element *galerkinElement) {
    pixelData = SglPixelContent::ELEMENT_POINTER;
    currentGalerkinElement = galerkinElement;
}

void
SglContext::sglViewport(int x, int y, int viewPortWidth, int viewPortHeight) {
    vp_x = x;
    vp_y = y;
    vp_width = viewPortWidth;
    vp_height = viewPortHeight;
}

void
SglContext::sglPolygon(const int numberOfVertices, const Vector3D *vertices) {
    Polygon pol{};
    Window win{};
    PolygonBox clip_box = {-1.0, 1.0, -1.0, 1.0, -1.0, 1.0};

    if ( numberOfVertices > (clipping ? (PolygonClipResultInfo::MAXIMUM_SIDES_PER_POLYGON - 6) : PolygonClipResultInfo::MAXIMUM_SIDES_PER_POLYGON) ) {
        Error::error("sglPolygon", "Too many vertices (max. %d)", PolygonClipResultInfo::MAXIMUM_SIDES_PER_POLYGON);
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
        pol.mask = Poly::mask(offsetof(PolygonVertex, sx)) |
                   Poly::mask(offsetof(PolygonVertex, sy)) |
                   Poly::mask(offsetof(PolygonVertex, sz)) |
                   Poly::mask(offsetof(PolygonVertex, sw));
        if ( Poly::clipToBox(&pol, &clip_box) == POLY_CLIP_OUT ) {
            return;
        }
    }

    // Perspective divide and transformation to viewport and depth range
    for ( int i = 0; i < pol.n; i++ ) {
        PolygonVertex &vertex = pol.vertices[i];
        vertex.sx = static_cast<double>(vp_x) + (vertex.sx / vertex.sw + 1.0) * static_cast<double>(vp_width) * 0.5;
        vertex.sy = static_cast<double>(vp_y) + (vertex.sy / vertex.sw + 1.0) * static_cast<double>(vp_height) * 0.5;
        vertex.sz = (near + (vertex.sz / vertex.sw + 1.0) * far * 0.5) * static_cast<double>(SglConstants::SGL_MAXIMUM_Z);
    }

    // Window
    win.x0 = vp_x;
    win.y0 = vp_y;
    win.x1 = vp_x + vp_width - 1;
    win.y1 = vp_y + vp_height - 1;

    // Scan convert the polygon: use optimized version for flat shading with or without Z buffering
    if ( depthBuffer != nullptr ) {
        Poly::scanZ(this, &pol, &win);
    } else {
        Poly::scanFlat(this, &pol, &win);
    }
}
