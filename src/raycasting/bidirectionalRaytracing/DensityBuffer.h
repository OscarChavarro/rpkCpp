/**
Density estimation on screen
*/

#ifndef __DENSITY_BUFFER__
#define __DENSITY_BUFFER__

#include "common/ColorRgb.h"
#include "render/ScreenBuffer.h"
#include "raycasting/bidirectionalRaytracing/BidirectionalPathRaytracerConfig.h"
#include "raycasting/bidirectionalRaytracing/DensityHitList.h"

const int DHA_X_RES = 50;
const int DHA_Y_RES = 50;  // Subdivide image plane for efficient hit searches

class DensityBuffer {
  private:
    // A matching screen buffer is kept. This one will be filled in
    // by density estimation...
    ScreenBuffer *screenBuffer;
    BidirectionalPathRaytracerConfig *baseConfig;
    float xMinimum; // Copy from screenBuffer
    float xMaximum;
    float yMinimum;
    float yMaximum;
    DensityHitList hitGrid[DHA_X_RES][DHA_Y_RES];

    inline int xIndex(float x) const;
    inline int yIndex(float y) const;

  public:
    DensityBuffer(ScreenBuffer *screen, BidirectionalPathRaytracerConfig *paramBaseConfig);
    ~DensityBuffer();
    void add(float x, float y, ColorRgb color);
    ScreenBuffer *reconstruct();
    ScreenBuffer *reconstructVariable(ScreenBuffer *dest, float baseSize = 4.0);
};

#endif
