//= References:                                                             =
//= [HECK1990] Heckbert, Paul S. "Generic Convex Polygon Scan Conversion   =
//=          and Clipping", Graphics Gems, ed. Andrew S. Glassner,         =
//=          Academic Press, 1990, section II.5, pp. 84-86.                 =
//= [BEKA1997] Bekaert, Philippe. "SGL: Simple (Small) Graphics Library", =
//=          SGL_README, June 20, 1997; RenderPark source, as preserved in =
//=          this repository at commit c98a00f1, doc/txt/SGL_README.       =
//= [SUTH1974b] Sutherland, Ivan E.; Hodgman, Gary W. "Reentrant Polygon    =
//=          Clipping", Communications of the ACM 17(1), 32-42, 1974.     =
//=          DOI: 10.1145/360767.360802.                                   =

/*
The polygon rasterizer integrates the clipping and scan conversion routines of
[HECK1990].II.5, with flat and depth-buffered scan conversion optimized by
[BEKA1997] (the SGL library of RenderPark). Heckbert credits the scan converter
to an algorithm by Frank Crow at NYIT and his clipper to an implementation of
[SUTH1974b] by Ed Catmull and Alvy Ray Smith. These two implementations are
identified in [HECK1990].85; no separate publication is specified there. The
half-space clipping is in convexPolygonClipper/ConvexWindowClipper, the box
clipping in PolygonBoxClipper and the scan conversion in PolygonScanConverter,
while this context prepares their projected input.
*/

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/render/raster/polygonScanConverter/PolygonBoxClipper.h"
#include "vsdk/toolkit/render/raster/polygonScanConverter/PolygonRasterContext.h"

PolygonRasterContext::PolygonRasterContext(int inWidth, int inHeight, BOOLEAN depthTesting):
    width(inWidth),
    height(inHeight),
    viewport(0, 0, inWidth - 1, inHeight - 1),
    transform(),
    depthBuffer(depthTesting ? new RasterDepthValue[(size_t)inWidth * inHeight] : (RasterDepthValue *)NULL)
{
}

PolygonRasterContext::~PolygonRasterContext() {
    delete[] depthBuffer;
}

void
PolygonRasterContext::setViewport(int x, int y, int viewportWidth, int viewportHeight) {
    if ( x < 0 || y < 0 || viewportWidth <= 0 || viewportHeight <= 0 ||
         viewportWidth > width - x || viewportHeight > height - y ) {
        Logger::reportMessage("PolygonRasterContext", Logger::ERROR, "setViewport", "Viewport lies outside the raster target");
        return;
    }

    viewport = RasterRectangle2D(x, y, x + viewportWidth - 1, y + viewportHeight - 1);
}

/**
Transforms, clips and projects the polygon to the viewport and depth range.
The result is empty when nothing of the polygon is to be drawn.
*/
ScreenPolygon
PolygonRasterContext::projectPolygon(int numberOfVertices, const Vector3Dd *vertices) const {
    const PolygonBox clipBox(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

    if ( numberOfVertices < 3 || numberOfVertices > ScreenPolygon::MAXIMUM_SIDES_PER_POLYGON - 6 ) {
        Logger::reportMessage("PolygonRasterContext", Logger::ERROR, "projectPolygon", "Invalid vertex count for polygon rasterization");
        return ScreenPolygon();
    }

    // Transform the vertices and fill in a ScreenPolygon
    int i;
    PolygonVertex transformed[ScreenPolygon::MAXIMUM_SIDES_PER_POLYGON];
    for ( i = 0; i < numberOfVertices; i++ ) {
        const Vector4Dd v = transform.multiply(
            Vector4Dd(vertices[i].x(), vertices[i].y(), vertices[i].z(), 1.0));
        transformed[i] = PolygonVertex(v.x(), v.y(), v.z(), v.w(), 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    }
    const unsigned long mask =
        ScreenPolygon::attributeBit(PolygonVertex::SX) |
        ScreenPolygon::attributeBit(PolygonVertex::SY) |
        ScreenPolygon::attributeBit(PolygonVertex::SZ) |
        ScreenPolygon::attributeBit(PolygonVertex::SW);
    // Clip homogeneous coordinates before perspective division, as in
    // [SUTH1974b], p. 33; the plane tests are in PolygonBoxClipper and the
    // edge cases in ConvexWindowClipper.
    const PolygonClipOutcome outcome =
        PolygonBoxClipper::clipToBox(ScreenPolygon(numberOfVertices, mask, transformed), clipBox);
    if ( outcome.getResult() == POLY_CLIP_OUT ) {
        return ScreenPolygon();
    }

    // Perspective divide and transformation to viewport and depth range
    const ScreenPolygon &clipped = outcome.getPolygon();
    PolygonVertex projected[ScreenPolygon::MAXIMUM_SIDES_PER_POLYGON];
    for ( i = 0; i < clipped.getNumberOfVertices(); i++ ) {
        const PolygonVertex &vertex = clipped.getVertex(i);
        if ( vertex.getSw() < VSDK::EPSILON ) {
            // Inside the clip box -w <= sx, sy, sz <= w, so w >= 0; a w near zero
            // leaves the vertex at the eye, where the perspective divide is
            // undefined. The test is done after clipping, so that a polygon that
            // only crosses the plane w = 0 keeps its visible part.
            return ScreenPolygon();
        }
        projected[i] = PolygonVertex(
            (double)viewport.getMinX() + (vertex.getSx() / vertex.getSw() + 1.0) * (double)getViewportWidth() * 0.5,
            (double)viewport.getMinY() + (vertex.getSy() / vertex.getSw() + 1.0) * (double)getViewportHeight() * 0.5,
            ((vertex.getSz() / vertex.getSw() + 1.0) * 0.5) * (double)PolygonScanConverterDepth::MAXIMUM_DEPTH,
            vertex.getSw(),
            vertex.getX(), vertex.getY(), vertex.getZ(),
            vertex.getU(), vertex.getV(),
            vertex.getR(), vertex.getG(), vertex.getB());
    }

    return ScreenPolygon(clipped.getNumberOfVertices(), clipped.getMask(), projected);
}
