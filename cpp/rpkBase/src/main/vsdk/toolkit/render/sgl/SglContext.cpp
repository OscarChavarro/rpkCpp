#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/render/sgl/Poly.h"
#include "vsdk/toolkit/render/sgl/SglContext.h"

SglContext::SglContext(int inWidth, int inHeight, bool depthTesting):
    width(inWidth),
    height(inHeight),
    viewport{0, 0, inWidth - 1, inHeight - 1},
    transform(),
    depthBuffer(depthTesting ? new SglDepthValue[static_cast<std::size_t>(inWidth) * inHeight] : nullptr)
{
}

SglContext::~SglContext() {
    delete[] depthBuffer;
}

void
SglContext::setViewport(int x, int y, int viewportWidth, int viewportHeight) {
    if ( x < 0 || y < 0 || viewportWidth <= 0 || viewportHeight <= 0 ||
         viewportWidth > width - x || viewportHeight > height - y ) {
        Logger::reportMessage("SglContext", Logger::ERROR, "setViewport", "Viewport lies outside the raster target");
        return;
    }

    viewport = Window{x, y, x + viewportWidth - 1, y + viewportHeight - 1};
}

bool
SglContext::projectPolygon(int numberOfVertices, const Vector3Dd *vertices, ScreenPolygon &pol) const {
    const PolygonBox clipBox = {-1.0, 1.0, -1.0, 1.0, -1.0, 1.0};

    if ( numberOfVertices < 3 || numberOfVertices > PolygonClipResultInfo::MAXIMUM_SIDES_PER_POLYGON - 6 ) {
        Logger::reportMessage("SglContext", Logger::ERROR, "projectPolygon", "Invalid vertex count for polygon rasterization");
        return false;
    }

    // Transform the vertices and fill in a Poly
    for ( int i = 0; i < numberOfVertices; i++ ) {
        PolygonVertex &vertex = pol.vertices[i];
        const Vector4Dd v = transform.multiply(
            Vector4Dd(vertices[i].x(), vertices[i].y(), vertices[i].z(), 1.0));
        if ( v.w() > -VSDK::EPSILON && v.w() < VSDK::EPSILON ) {
            return false;
        }
        vertex.sx = v.x();
        vertex.sy = v.y();
        vertex.sz = v.z();
        vertex.sw = v.w();
    }
    pol.n = numberOfVertices;
    pol.mask = Poly::mask(offsetof(PolygonVertex, sx)) |
               Poly::mask(offsetof(PolygonVertex, sy)) |
               Poly::mask(offsetof(PolygonVertex, sz)) |
               Poly::mask(offsetof(PolygonVertex, sw));
    if ( Poly::clipToBox(&pol, &clipBox) == POLY_CLIP_OUT ) {
        return false;
    }

    // Perspective divide and transformation to viewport and depth range
    for ( int i = 0; i < pol.n; i++ ) {
        PolygonVertex &vertex = pol.vertices[i];
        vertex.sx = static_cast<double>(viewport.x0) + (vertex.sx / vertex.sw + 1.0) * static_cast<double>(getViewportWidth()) * 0.5;
        vertex.sy = static_cast<double>(viewport.y0) + (vertex.sy / vertex.sw + 1.0) * static_cast<double>(getViewportHeight()) * 0.5;
        vertex.sz = ((vertex.sz / vertex.sw + 1.0) * 0.5) * static_cast<double>(SglConstants::SGL_MAXIMUM_Z);
    }

    return true;
}
