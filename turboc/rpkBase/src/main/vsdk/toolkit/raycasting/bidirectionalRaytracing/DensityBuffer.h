#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

/**
Density estimation on screen
*/

#ifndef __DENSITY_BUFFER__
#define __DENSITY_BUFFER__

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/render/ScreenBuffer.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/BidirectionalPathRaytracerConfig.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/DensityHitList.h"

class DensityBuffer {
  private:
    static const int DHA_X_RES = 50;
    static const int DHA_Y_RES = 50; // Subdivide image plane for efficient hit searches

    // A matching screen buffer is kept. This one will be filled in
    // by density estimation...
    ScreenBuffer *screenBuffer;
    BidirPathRaytrcCnfg *baseConfig;
    double xMinimum; // Copy from screenBuffer
    double xMaximum;
    double yMinimum;
    double yMaximum;
    DensityHitList hitGrid[DHA_X_RES][DHA_Y_RES];

    inline int xIndex(double x) const;
    inline int yIndex(double y) const;

  public:
    DensityBuffer(ScreenBuffer *screen, BidirPathRaytrcCnfg *paramBaseConfig);
    ~DensityBuffer();
    void add(double x, double y, ColorRgb color);
    ScreenBuffer *reconstruct();
    ScreenBuffer *reconstructVariable(ScreenBuffer *dest, double baseSize = 4.0);
};

#endif
