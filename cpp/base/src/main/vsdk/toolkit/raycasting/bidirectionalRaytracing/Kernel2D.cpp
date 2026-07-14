/**
Density kernel functions
Many routines borrowed from Density Estimation master thesis by
Olivier Ceulemans.
*/
#include "java/lang/System.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/Kernel2D.h"

#ifdef RAYTRACING_ENABLED
Kernel2D::Kernel2D() {
    Init(1.0, 1.0);
}

void
Kernel2D::Init(float h, float w) {
    m_h = h;
    m_weight = w;

    m_h2 = h * h;
    m_h2inv = 1 / m_h2;
}

/**
Change kernel width.
IN:   The new kernel width, newH.
PRE:  newH > 0.0F
POST: The kernel size is newH.
*/
void
Kernel2D::SetH(const float newH) {
    Init(newH, m_weight);
}

/**
Evaluate the kernel
IN:  The position of a 2D point.
     The position of the center of the kernel.
OUT: The value of the kernel at the point. (a positive number or 0.0F)
*/
float
Kernel2D::Evaluate(const Vector2D &point, const Vector2D &center) const {
    Vector2D aux;
    float tp;

    // Find distance
    Vector2D::difference(point, center, aux);
    tp = Vector2D::norm2(aux);

    if ( tp < m_h2 ) {
        // Point inside kernel
        tp = (1.0F - (tp * m_h2inv));
        tp = static_cast<float>(M_2_PI) * tp * m_h2inv;
        return tp;
    } else {
        return 0.0F;
    }
}

/**
cover the affected pixels of a screen buffer with the kernel
IN: The screen buffer to cover.
OUT: /
*/
void
Kernel2D::cover(const Vector2D &point, float scale, const ColorRgbMutable &col, ScreenBuffer *screen) const {
    // For each neighbourhood pixel : eval kernel and add contrib

    int nxMin;
    int nxMax;
    int nyMin;
    int nyMax;
    Vector2D center;
    ColorRgbMutable addCol(0.0, 0.0, 0.0);
    float factor;

    // Get extents of possible pixels that are affected
    screen->getPixel(point.u - m_h, point.v - m_h, &nxMin, &nyMin);
    screen->getPixel(point.u + m_h, point.v + m_h, &nxMax, &nyMax);

    for ( int nx = nxMin; nx <= nxMax; nx++ ) {
        for ( int ny = nyMin; ny <= nyMax; ny++ ) {
            if ( (nx >= 0) && (ny >= 0) && (nx < screen->getHRes()) && (ny < screen->getVRes()) ) {
                center = screen->getPixelCenter(nx, ny);
                factor = scale * Evaluate(point, center);
                addCol.scaledCopy(factor, col);
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
    const Vector2D &center,
    const ColorRgbMutable &color,
    const ScreenBuffer *ref,
    ScreenBuffer *dest,
    int totalSamples,
    int scaleSamples,
    float baseSize)
{
    float screenScale = java::Math::max(ref->getPixXSize(), ref->getPixYSize());
    float B = baseSize * screenScale; // what about the 8 ??

    // Use optimal N (samples per pixel) dependency for fixed kernels
    // scaleSamples is normally total samples per pixel, while
    // totalSamples is the total number of samples for the CURRENT
    // number of samples per pixel
    float Bn = static_cast<float>(B * (java::Math::pow(static_cast<double>(scaleSamples), (-1.5 / 5.0))));

    float h;

    // Now compute h for this sample

    // Reference estimated function
    ColorRgbMutable fe = ref->getBiLinear(center.u, center.v);

    float avgFe = fe.average();
    float avgG = color.average();

    if ( avgFe > Numeric::EPSILON ) {
        h = Bn * java::Math::sqrt(avgG / avgFe);
        // printf("fe %f G %f, h = %f\n", avgFe, avgG, h/screenScale);
    } else {
        const float maxRatio = 20; // ???
        h = Bn * maxRatio * screenScale;

        java::System::out.printf("MaxRatio... h = %f\n", h / screenScale);
    }

    h = java::Math::max(1.0F * screenScale, h); // We want to cover at least one pixel...

    SetH(h);

    // h determined, now splat the fucker
    cover(center, 1.0F / static_cast<float>(totalSamples), color, dest);
}
#endif
