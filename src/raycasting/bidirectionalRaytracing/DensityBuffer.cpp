#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "java/lang/Math.h"
#include "raycasting/bidirectionalRaytracing/DensityBuffer.h"
#include "raycasting/bidirectionalRaytracing/densitykernel.h"

inline int
DensityBuffer::xIndex(float x) const {
    return java::Math::min(
        (int)(DHA_X_RES * (x - xMinimum) / (xMaximum - xMinimum)),
        DHA_X_RES - 1);
}

inline int
DensityBuffer::yIndex(float y) const {
    return java::Math::min(
            (int)(DHA_Y_RES * (y - yMinimum) / (yMaximum - yMinimum)),
            DHA_Y_RES - 1);
}

DensityBuffer::DensityBuffer(ScreenBuffer *screen, BidirectionalPathRaytracerConfig *paramBaseConfig) {
    screenBuffer = screen;
    baseConfig = paramBaseConfig;

    xMinimum = screenBuffer->getScreenXMin();
    xMaximum = screenBuffer->getScreenXMax();
    yMinimum = screenBuffer->getScreenYMin();
    yMaximum = screenBuffer->getScreenYMax();

    printf("Density Buffer :\nXmin %f, Ymin %f, Xmax %f, Ymax %f\n",
           xMinimum, yMinimum, xMaximum, yMaximum);

}

DensityBuffer::~DensityBuffer() {
}

/**
Add a hit
*/
void
DensityBuffer::add(float x, float y, ColorRgb color) {
    float factor = screenBuffer->getPixXSize() * screenBuffer->getPixYSize()
                   * (float) baseConfig->totalSamples;
    ColorRgb tmpCol;

    if ( color.average() > Numeric::EPSILON ) {
        tmpCol.scaledCopy(factor, color); // Undo part of flux to rad factor

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
    float h = 8.0f * java::Math::max(screenBuffer->getPixXSize(), screenBuffer->getPixYSize())
              / java::Math::sqrt((float)baseConfig->samplesPerPixel);

    printf("h = %f\n", h);

    screenBuffer->scaleRadiance(0.0); // Hack!

    int maxK;
    DensityHit hit;
    CKernel2D kernel;
    Vector2D center;

    kernel.SetH(h);

    for ( int i = 0; i < DHA_X_RES; i++ ) {
        for ( int j = 0; j < DHA_Y_RES; j++ ) {
            maxK = hitGrid[i][j].storedHits();

            for ( int k = 0; k < maxK; k++ ) {
                hit = (hitGrid[i][j])[k];

                center.u = hit.m_x;
                center.v = hit.m_y;

                kernel.cover(center, 1.0f / (float) baseConfig->totalSamples, hit.color, screenBuffer);
            }
        }
    }

    return screenBuffer;
}


ScreenBuffer *
DensityBuffer::reconstructVariable(ScreenBuffer *dest, float baseSize) {
    // For all samples -> compute pixel coverage

    // Base Kernel size. Now spread over a number of pixels

    dest->scaleRadiance(0.0); // Hack!

    int maxK;
    DensityHit hit;
    CKernel2D kernel;
    Vector2D center;

    for ( int i = 0; i < DHA_X_RES; i++ ) {
        for ( int j = 0; j < DHA_Y_RES; j++ ) {
            maxK = hitGrid[i][j].storedHits();

            for ( int k = 0; k < maxK; k++ ) {
                hit = (hitGrid[i][j])[k];

                center.u = hit.m_x;
                center.v = hit.m_y;

                kernel.varCover(center, hit.color, screenBuffer, dest, (int) baseConfig->totalSamples,
                                baseConfig->samplesPerPixel, baseSize);
            }
        }
    }

    return dest;
}

#endif
