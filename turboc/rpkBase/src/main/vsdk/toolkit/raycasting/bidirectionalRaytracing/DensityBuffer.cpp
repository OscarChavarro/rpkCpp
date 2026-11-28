#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/skin/Numeric.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "java/lang/System.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/DensityBuffer.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/Kernel2D.h"

inline int
DensityBuffer::xIndex(double x) const {
    return Math::min(
        ((int)(DensityBuffer::DHA_X_RES * (x - xMinimum) / (xMaximum - xMinimum))),
        DensityBuffer::DHA_X_RES - 1);
}

inline int
DensityBuffer::yIndex(double y) const {
    return Math::min(
            ((int)(DensityBuffer::DHA_Y_RES * (y - yMinimum) / (yMaximum - yMinimum))),
            DensityBuffer::DHA_Y_RES - 1);
}

DensityBuffer::DensityBuffer(ScreenBuffer *screen, BidirPathRaytrcCnfg *paramBaseConfig) {
    screenBuffer = screen;
    baseConfig = paramBaseConfig;

    xMinimum = screenBuffer->getScreenXMin();
    xMaximum = screenBuffer->getScreenXMax();
    yMinimum = screenBuffer->getScreenYMin();
    yMaximum = screenBuffer->getScreenYMax();

    System::out.printf("Density Buffer :\nXmin %f, Ymin %f, Xmax %f, Ymax %f\n",
           xMinimum, yMinimum, xMaximum, yMaximum);

}

DensityBuffer::~DensityBuffer() {
}

/**
Add a hit
*/
void
DensityBuffer::add(double x, double y, ColorRgb color) {
    double factor = screenBuffer->getPixXSize() * screenBuffer->getPixYSize()
                   * ((double)(baseConfig->totalSamples));
    ColorRgb tmpCol;

    if ( ColorRgbMutable(color).average() > VSDK::EPSILON ) {
        tmpCol = ColorRgb(
            factor * color.getR(),
            factor * color.getG(),
            factor * color.getB()); // Undo part of flux to rad factor

        DensityHit hit(x, y, tmpCol);

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
    double h = 8.0 * Math::max(screenBuffer->getPixXSize(), screenBuffer->getPixYSize())
              / Math::sqrt(((double)(baseConfig->samplesPerPixel)));

    System::out.printf("h = %f\n", h);

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

                kernel.cover(center, 1.0 / ((double)(baseConfig->totalSamples)), hit.color, screenBuffer);
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

                kernel.varCover(center, hit.color, screenBuffer, dest, ((int)(baseConfig->totalSamples)),
                                baseConfig->samplesPerPixel, baseSize);
            }
        }
    }

    return dest;
}

#endif
