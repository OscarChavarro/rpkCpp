#ifndef POLYGON_VISIBILITY_BUFFER_TXX__
#define POLYGON_VISIBILITY_BUFFER_TXX__

/**
Implementation of PolygonVisibilityBuffer, generic on its Sample payload. It does
not know which payloads exist: each client module that defines one includes
this file in a single translation unit of its own and instantiates it there,
next to the extern template declaration of its alias. That keeps the dependency pointing from the client to this package, and the scan conversion
implementation out of every other translation unit.
*/

#include "vsdk/toolkit/render/raster/polygonScanConverter/PolygonVisibilityBuffer.h"
#include "vsdk/toolkit/render/raster/polygonScanConverter/PolygonScanFlat.txx"
#include "vsdk/toolkit/render/raster/polygonScanConverter/PolygonScanZ.txx"

template<class Sample>
PolygonVisibilityBuffer<Sample>::PolygonVisibilityBuffer(int width, int height, Sample backgroundValue, bool depthTesting):
    context(width, height, depthTesting),
    background(backgroundValue),
    samples(new Sample[static_cast<std::size_t>(width) * height])
{
    clear();
}

template<class Sample>
PolygonVisibilityBuffer<Sample>::~PolygonVisibilityBuffer() {
    delete[] samples;
}

template<class Sample>
void
PolygonVisibilityBuffer<Sample>::clear() {
    const RasterRectangle2D &viewport = getViewport();
    RasterDepthValue *const depth = context.getDepthBuffer();
    for ( int y = viewport.getMinY(); y <= viewport.getMaxY(); y++ ) {
        const int firstSample = y * getWidth() + viewport.getMinX();
        const int endSample = firstSample + getViewportWidth();
        for ( int i = firstSample; i < endSample; i++ ) {
            samples[i] = background;
        }
        if ( depth != nullptr ) {
            for ( int i = firstSample; i < endSample; i++ ) {
                depth[i] = PolygonScanConverter::MAXIMUM_DEPTH;
            }
        }
    }
}

template<class Sample>
void
PolygonVisibilityBuffer<Sample>::drawPolygon(int numberOfVertices, const Vector3Dd *vertices, Sample value) {
    const ScreenPolygon polygon = context.projectPolygon(numberOfVertices, vertices);
    if ( polygon.getNumberOfVertices() == 0 ) {
        return;
    }

    RasterDepthValue *const depth = context.getDepthBuffer();
    if ( depth != nullptr ) {
        PolygonScanConverter::scanZ(samples, depth, getWidth(), value, &polygon, &getViewport());
    } else {
        PolygonScanConverter::scanFlat(samples, getWidth(), value, &polygon, &getViewport());
    }
}

#endif
