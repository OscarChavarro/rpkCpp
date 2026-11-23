#include "vsdk/toolkit/galerkin/GalerkinElementVisibilityBuffer.h"
#include "vsdk/toolkit/render/FaceVisibilityBuffer.h"
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
    Polygon polygon{};
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

// Keep the scan conversion implementation private to this translation unit.
template class SglVisibilityBuffer<SglFaceIndex>;
template class SglVisibilityBuffer<const GalerkinElement *>;
