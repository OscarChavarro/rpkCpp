#ifndef SGL_VISIBILITY_BUFFER_TXX__
#define SGL_VISIBILITY_BUFFER_TXX__

/**
Implementation of SglVisibilityBuffer, generic on its Sample payload. sgl does
not know which payloads exist: each client module that defines one includes
this file in a single translation unit of its own and instantiates it there
(see FaceVisibilityBuffer.cpp and GalerkinElementVisibilityBuffer.cpp). That
keeps the dependency pointing from the client to sgl, and the scan conversion
implementation out of every other translation unit.
*/

#include "vsdk/toolkit/render/sgl/SglVisibilityBuffer.h"
#include "vsdk/toolkit/render/sgl/PolyScanFlat.txx"
#include "vsdk/toolkit/render/sgl/PolyScanZ.txx"

template<class Sample>
SglVisibilityBuffer<Sample>::SglVisibilityBuffer(int width, int height, Sample backgroundValue, bool depthTesting):
    context(width, height, depthTesting),
    background(backgroundValue),
    samples(new Sample[static_cast<std::size_t>(width) * height])
{
    clear();
}

template<class Sample>
SglVisibilityBuffer<Sample>::~SglVisibilityBuffer() {
    delete[] samples;
}

template<class Sample>
void
SglVisibilityBuffer<Sample>::clear() {
    const Window &viewport = getViewport();
    SglDepthValue *const depth = context.getDepthBuffer();
    for ( int y = viewport.y0; y <= viewport.y1; y++ ) {
        const int firstSample = y * getWidth() + viewport.x0;
        const int endSample = firstSample + getViewportWidth();
        for ( int i = firstSample; i < endSample; i++ ) {
            samples[i] = background;
        }
        if ( depth != nullptr ) {
            for ( int i = firstSample; i < endSample; i++ ) {
                depth[i] = SglConstants::SGL_MAXIMUM_Z;
            }
        }
    }
}

template<class Sample>
void
SglVisibilityBuffer<Sample>::drawPolygon(int numberOfVertices, const Vector3Df *vertices, Sample value) {
    ScreenPolygon polygon{};
    if ( !context.projectPolygon(numberOfVertices, vertices, polygon) ) {
        return;
    }

    SglDepthValue *const depth = context.getDepthBuffer();
    if ( depth != nullptr ) {
        Poly::scanZ(samples, depth, getWidth(), value, &polygon, &getViewport());
    } else {
        Poly::scanFlat(samples, getWidth(), value, &polygon, &getViewport());
    }
}

#endif
