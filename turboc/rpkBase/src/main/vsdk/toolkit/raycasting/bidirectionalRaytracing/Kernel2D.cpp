/**
Density kernel functions
Many routines borrowed from Density Estimation master thesis by
Olivier Ceulemans.
*/
#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "java/lang/System.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/Kernel2D.h"

#ifdef RAYTRACING_ENABLED
Kernel2D::Kernel2D() {
    Init(1.0, 1.0);
}

void
Kernel2D::Init(double h, double w) {
    m_h = h;
    m_weight = w;

    m_h2 = h * h;
    m_h2inv = 1 / m_h2;
}

/**
Change kernel width.
IN:   The new kernel width, newH.
PRE:  newH > 0.0
POST: The kernel size is newH.
*/
void
Kernel2D::SetH(const double newH) {
    Init(newH, m_weight);
}

/**
Evaluate the kernel
IN:  The position of a 2D point.
     The position of the center of the kernel.
OUT: The value of the kernel at the point. (a positive number or 0.0)
*/
double
Kernel2D::Evaluate(const Vector2Dd &point, const Vector2Dd &center) const {
    // Find distance
    double tp = point.subtract(center).lengthSquared();

    if ( tp < m_h2 ) {
        // Point inside kernel
        tp = (1.0 - (tp * m_h2inv));
        tp = ((double)(M_2_PI)) * tp * m_h2inv;
        return tp;
    } else {
        return 0.0;
    }
}

/**
cover the affected pixels of a screen buffer with the kernel
IN: The screen buffer to cover.
OUT: /
*/
void
Kernel2D::cover(const Vector2Dd &point, double scale, const ColorRgb &col, ScreenBuffer *screen) const {
    // For each neighbourhood pixel : eval kernel and add contrib

    int nxMin;
    int nxMax;
    int nyMin;
    int nyMax;
    Vector2Dd center;
    ColorRgb addCol;
    double factor;

    // Get extents of possible pixels that are affected
    screen->getPixel(point.x - m_h, point.y - m_h, &nxMin, &nyMin);
    screen->getPixel(point.x + m_h, point.y + m_h, &nxMax, &nyMax);

    for ( int nx = nxMin; nx <= nxMax; nx++ ) {
        for ( int ny = nyMin; ny <= nyMax; ny++ ) {
            if ( (nx >= 0) && (ny >= 0) && (nx < screen->getHRes()) && (ny < screen->getVRes()) ) {
                center = screen->getPixelCenter(nx, ny);
                factor = scale * Evaluate(point, center);
                addCol = ColorRgb(
                    factor * col.getR(),
                    factor * col.getG(),
                    factor * col.getB());
                screen->add(nx, ny, addCol);
            } else {
                // Handle boundary bias !
            }
        }
    }
}

/**
Add one hit/splat with a size dependend on a reference estimate
*/
void
Kernel2D::varCover(
    const Vector2Dd &center,
    const ColorRgb &color,
    const ScreenBuffer *ref,
    ScreenBuffer *dest,
    int totalSamples,
    int scaleSamples,
    double baseSize)
{
    double screenScale = Math::max(ref->getPixXSize(), ref->getPixYSize());
    double B = baseSize * screenScale; // what about the 8 ??

    // Use optimal N (samples per pixel) dependency for fixed kernels
    // scaleSamples is normally total samples per pixel, while
    // totalSamples is the total number of samples for the CURRENT
    // number of samples per pixel
    double Bn = ((double)(B * (Math::pow(((double)(scaleSamples)), (-1.5 / 5.0)))));

    double h;

    // Now compute h for this sample

    // Reference estimated function
    ColorRgb fe = ref->getBiLinear(center.x, center.y);

    double avgFe = ColorRgbMutable(fe).average();
    double avgG = ColorRgbMutable(color).average();

    if ( avgFe > VSDK::EPSILON ) {
        h = Bn * Math::sqrt(avgG / avgFe);
        // printf("fe %f G %f, h = %f\n", avgFe, avgG, h/screenScale);
    } else {
        const double maxRatio = 20; // ???
        h = Bn * maxRatio * screenScale;

        System::out.printf("MaxRatio... h = %f\n", h / screenScale);
    }

    h = Math::max(1.0 * screenScale, h); // We want to cover at least one pixel...

    SetH(h);

    // h determined, now splat the fucker
    cover(center, 1.0 / ((double)(totalSamples)), color, dest);
}
#endif
