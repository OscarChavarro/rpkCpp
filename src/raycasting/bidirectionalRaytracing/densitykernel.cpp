/**
Density kernel functions
Many routines borrowed from Density Estimation master thesis by
Olivier Ceulemans.
*/

#include "java/lang/Math.h"
#include "raycasting/bidirectionalRaytracing/densitykernel.h"

#ifdef RAYTRACING_ENABLED
CKernel2D::CKernel2D() {
    Init(1.0, 1.0);
}

void
CKernel2D::Init(float h, float w) {
    m_h = h;
    m_weight = w;

    m_h2 = h * h;
    m_h2inv = 1 / m_h2;
}

/**
Change kernel width.
IN:   The new kernel width, newH.
PRE:  newH > 0.0f
POST: The kernel size is newH.
*/
void
CKernel2D::SetH(const float newH) {
    Init(newH, m_weight);
}

/**
Evaluate the kernel
IN:  The position of a 2D point.
     The position of the center of the kernel.
OUT: The value of the kernel at the point. (a positive number or 0.0f)
*/
float
CKernel2D::Evaluate(const Vector2D &point, const Vector2D &center) const {
    Vector2D aux;
    float tp;

    // Find distance
    vector2DDifference(point, center, aux);
    tp = vector2DNorm2(aux);

    if ( tp < m_h2 ) {
        // Point inside kernel
        tp = (1.0f - (tp * m_h2inv));
        tp = (float)M_2_PI * tp * m_h2inv;
        return tp;
    } else {
        return 0.0f;
    }
}

/**
cover the affected pixels of a screen buffer with the kernel
IN: The screen buffer to cover.
OUT: /
*/
void
CKernel2D::cover(const Vector2D &point, float scale, const ColorRgb &col, ScreenBuffer *screen) const {
    // For each neighbourhood pixel : eval kernel and add contrib

    int nxMin;
    int nxMax;
    int nyMin;
    int nyMax;
    Vector2D center;
    ColorRgb addCol;
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
CKernel2D::varCover(
    const Vector2D &center,
    const ColorRgb &color,
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
    float Bn = (float)(B * (java::Math::pow((double) scaleSamples, (-1.5 / 5.0))));

    float h;

    // Now compute h for this sample

    // Reference estimated function
    ColorRgb fe = ref->getBiLinear(center.u, center.v);

    float avgFe = fe.average();
    float avgG = color.average();

    if ( avgFe > Numeric::EPSILON ) {
        h = Bn * java::Math::sqrt(avgG / avgFe);
        // printf("fe %f G %f, h = %f\n", avgFe, avgG, h/screenScale);
    } else {
        const float maxRatio = 20; // ???
        h = Bn * maxRatio * screenScale;

        printf("MaxRatio... h = %f\n", h / screenScale);
    }

    h = java::Math::max(1.0f * screenScale, h); // We want to cover at least one pixel...

    SetH(h);

    // h determined, now splat the fucker
    cover(center, 1.0f / (float) totalSamples, color, dest);
}
#endif
