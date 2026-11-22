#ifndef DENSITY_HIT__
#define DENSITY_HIT__

#include "vsdk/toolkit/common/color/ColorRgbMutable.h"

/**
Class DensityBuffer : class for storing sample hits on screen
New samples are added with 'Add'. 'reconstruct' reconstructs
an approximation to the sampled function into a screen buffer
*/
class DensityHit {
  public:
    float m_x; // Screen/Polygon Coordinates
    float m_y;
    ColorRgbMutable color; // Estimate of the function, NOT divided by number of samples

    DensityHit();
    void init(float x, float y, ColorRgbMutable col);
    explicit DensityHit(float x, float y, ColorRgbMutable col);
};

inline
DensityHit::DensityHit(): m_x(), m_y() {
}

inline
DensityHit::DensityHit(float x, float y, ColorRgbMutable col): m_x(), m_y() {
    init(x, y, col);
}

inline void
DensityHit::init(float x, float y, ColorRgbMutable col) {
    m_x = x;
    m_y = y;
    color = col;
}

#endif
