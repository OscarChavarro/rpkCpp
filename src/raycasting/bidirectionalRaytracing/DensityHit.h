#ifndef __DENSITY_HIT__
#define __DENSITY_HIT__

#include "common/ColorRgb.h"

/**
Class DensityBuffer : class for storing sample hits on screen
New samples are added with 'Add'. 'reconstruct' reconstructs
an approximation to the sampled function into a screen buffer
*/
class DensityHit {
  public:
    float m_x; // Screen/Polygon Coordinates
    float m_y;
    ColorRgb color; // Estimate of the function, NOT divided by number of samples

    inline void
    init(float x, float y, ColorRgb col) {
        m_x = x;
        m_y = y;
        color = col;
    }

    DensityHit(): m_x(), m_y() {}
    explicit DensityHit(float x, float y, ColorRgb col): m_x(), m_y() {
        init(x, y, col);
    }
};

#endif
