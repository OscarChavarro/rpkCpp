/**
Many routines borrowed from Density Estimation master thesis by Olivier Ceulemans.
*/

#ifndef __DENSITY_KERNEL__
#define __DENSITY_KERNEL__

#include "common/linealAlgebra/Vector2D.h"
#include "render/ScreenBuffer.h"

class CKernel2D {
protected:
    float m_h; // Kernel size.
    float m_h2; // h2=h*h.
    float m_h2inv; // h2inv=1/h2.
    float m_weight; // The weight of the kernel

public:
    CKernel2D();

    void Init(float h, float w);

    void SetH(float newH);

    float Evaluate(const Vector2D &point, const Vector2D &center) const;

    void cover(const Vector2D &point, float scale, const ColorRgb &col, ScreenBuffer *screen) const;

    void
    varCover(
        const Vector2D &center,
        ColorRgb &color,
        ScreenBuffer *ref,
        ScreenBuffer *dest,
        int totalSamples,
        int scaleSamples,
        float baseSize = 4.0);
};

#endif
