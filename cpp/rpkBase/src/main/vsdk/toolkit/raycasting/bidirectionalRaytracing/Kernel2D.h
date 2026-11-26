/**
Many routines borrowed from Density Estimation master thesis by Olivier Ceulemans.
*/

#ifndef __KERNEL_2_D__
#define __KERNEL_2_D__

#include "vsdk/toolkit/common/linealAlgebra/Vector2Dd.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/render/ScreenBuffer.h"

class Kernel2D final {
  private:
    double m_h; // Kernel size.
    double m_h2; // h2=h*h.
    double m_h2inv; // h2inv=1/h2.
    double m_weight; // The weight of the kernel

  public:
    Kernel2D();

    void Init(double h, double w);
    void SetH(double newH);
    double Evaluate(const Vector2Dd &point, const Vector2Dd &center) const;
    void cover(const Vector2Dd &point, double scale, const ColorRgb &col, ScreenBuffer *screen) const;

    void
    varCover(
        const Vector2Dd &center,
        const ColorRgb &color,
        const ScreenBuffer *ref,
        ScreenBuffer *dest,
        int totalSamples,
        int scaleSamples,
        double baseSize = 4.0);
};

#endif
