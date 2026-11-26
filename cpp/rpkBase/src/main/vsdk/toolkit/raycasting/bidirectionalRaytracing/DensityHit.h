#ifndef __DENSITY_HIT__
#define __DENSITY_HIT__

#include "vsdk/toolkit/common/color/ColorRgb.h"

/**
Class DensityBuffer : class for storing sample hits on screen
New samples are added with 'Add'. 'reconstruct' reconstructs
an approximation to the sampled function into a screen buffer
*/
class DensityHit {
  public:
    double m_x; // Screen/Polygon Coordinates
    double m_y;
    ColorRgb color; // Estimate of the function, NOT divided by number of samples

    DensityHit();
    void init(double x, double y, const ColorRgb &col);
    explicit DensityHit(double x, double y, const ColorRgb &col);
};

inline
DensityHit::DensityHit(): m_x(), m_y() {
}

inline
DensityHit::DensityHit(double x, double y, const ColorRgb &col): m_x(), m_y() {
    init(x, y, col);
}

inline void
DensityHit::init(double x, double y, const ColorRgb &col) {
    m_x = x;
    m_y = y;
    color = col;
}

#endif
