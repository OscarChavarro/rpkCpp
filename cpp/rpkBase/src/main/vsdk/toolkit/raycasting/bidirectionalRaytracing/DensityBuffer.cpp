#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "java/lang/System.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/DensityBuffer.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/Kernel2D.h"

inline int
DensityBuffer::xIndex(double x) const {
    return java::Math::min(
        static_cast<int>(DensityBuffer::DHA_X_RES * (x - xMinimum) / (xMaximum - xMinimum)),
        DensityBuffer::DHA_X_RES - 1);
}

inline int
DensityBuffer::yIndex(double y) const {
    return java::Math::min(
            static_cast<int>(DensityBuffer::DHA_Y_RES * (y - yMinimum) / (yMaximum - yMinimum)),
            DensityBuffer::DHA_Y_RES - 1);
}

DensityBuffer::DensityBuffer(ScreenBuffer *screen, BidirectionalPathRaytracerConfig *paramBaseConfig) {
    screenBuffer = screen;
    baseConfig = paramBaseConfig;

    xMinimum = screenBuffer->getScreenXMin();
    xMaximum = screenBuffer->getScreenXMax();
    yMinimum = screenBuffer->getScreenYMin();
    yMaximum = screenBuffer->getScreenYMax();

    java::System::out.printf("Density Buffer :\nXmin %f, Ymin %f, Xmax %f, Ymax %f\n",
           xMinimum, yMinimum, xMaximum, yMaximum);

}

DensityBuffer::~DensityBuffer() {
}

/**
Add a hit
*/
void
DensityBuffer::add(double x, double y, const ColorRgb &color) {
    double factor = screenBuffer->getPixXSize() * screenBuffer->getPixYSize()
                   * static_cast<double>(baseConfig->totalSamples);
    if ( color.average() > VSDK::EPSILON ) {
        const ColorRgb scaledColor = color.multiply(factor); // Undo part of flux to rad factor

        DensityHit hit(x, y, scaledColor);

        hitGrid[xIndex(x)][yIndex(y)].add(hit);
    }
}

/**
Reconstruct the internal screen buffer using constant kernel width
*/
ScreenBuffer *
DensityBuffer::reconstruct() {
    // For all samples -> compute pixel coverage

    // Kernel size. Now spread over 3 pixels
    double h = 8.0 * java::Math::max(screenBuffer->getPixXSize(), screenBuffer->getPixYSize())
              / java::Math::sqrt(static_cast<double>(baseConfig->samplesPerPixel));

    java::System::out.printf("h = %f\n", h);

    screenBuffer->scaleRadiance(0.0); // Hack!

    int maxK;
    DensityHit hit;
    Kernel2D kernel;
    Vector2Dd center;

    kernel.SetH(h);

    for ( int i = 0; i < DensityBuffer::DHA_X_RES; i++ ) {
        for ( int j = 0; j < DensityBuffer::DHA_Y_RES; j++ ) {
            maxK = hitGrid[i][j].storedHits();

            for ( int k = 0; k < maxK; k++ ) {
                hit = (hitGrid[i][j])[k];

                center = Vector2Dd(hit.m_x, hit.m_y);

                const ColorRgb color(hit.color.getR(), hit.color.getG(), hit.color.getB());
                kernel.cover(center, 1.0 / static_cast<double>(baseConfig->totalSamples), color, screenBuffer);
            }
        }
    }

    return screenBuffer;
}


ScreenBuffer *
DensityBuffer::reconstructVariable(ScreenBuffer *dest, double baseSize) {
    // For all samples -> compute pixel coverage

    // Base Kernel size. Now spread over a number of pixels

    dest->scaleRadiance(0.0); // Hack!

    int maxK;
    DensityHit hit;
    Kernel2D kernel;
    Vector2Dd center;

    for ( int i = 0; i < DensityBuffer::DHA_X_RES; i++ ) {
        for ( int j = 0; j < DensityBuffer::DHA_Y_RES; j++ ) {
            maxK = hitGrid[i][j].storedHits();

            for ( int k = 0; k < maxK; k++ ) {
                hit = (hitGrid[i][j])[k];

                center = Vector2Dd(hit.m_x, hit.m_y);

                const ColorRgb color(hit.color.getR(), hit.color.getG(), hit.color.getB());
                kernel.varCover(center, color, screenBuffer, dest, static_cast<int>(baseConfig->totalSamples),
                                baseConfig->samplesPerPixel, baseSize);
            }
        }
    }

    return dest;
}

#endif
