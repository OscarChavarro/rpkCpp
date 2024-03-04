/**
Density kernel functions
Many routines borrowed from Density Estimation master thesis by
Olivier Ceulemans.
*/

#include "common/mymath.h"
#include "raycasting/raytracing/densitykernel.h"

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

    // Epanechnikov kernel

    // Find distance
    vector2DDifference(point, center, aux);
    tp = vector2DNorm2(aux);

    if ( tp < m_h2 ) {
        // Point inside kernel
        tp = (1.0f - (tp * m_h2inv));
        tp = M_2_PI * tp * m_h2inv;
        return tp;
    } else {
        return 0.0f;
    }
}

/**
Cover the affected pixels of a screen buffer with the kernel
IN: The screen buffer to cover.
OUT: /
*/
void
CKernel2D::Cover(const Vector2D &point, float scale, COLOR &col, ScreenBuffer *screen) const {
    // For each neighbourhood pixel : eval kernel and add contrib

    int nx_min, nx_max, ny_min, ny_max, nx, ny;
    Vector2D center;
    COLOR addCol;
    float factor;

    //  screen->GetPixel(point, &nx, &ny);
    //screen->Add(nx, ny, col);
    //return;

    // Get extents of possible pixels that are affected
    screen->getPixel(point.u - m_h, point.v - m_h, &nx_min, &ny_min);
    screen->getPixel(point.u + m_h, point.v + m_h, &nx_max, &ny_max);

    for ( nx = nx_min; nx <= nx_max; nx++ ) {
        for ( ny = ny_min; ny <= ny_max; ny++ ) {
            if ((nx >= 0) && (ny >= 0) && (nx < screen->getHRes()) &&
                (ny < screen->getVRes())) {
                center = screen->getPixelCenter(nx, ny);
                factor = scale * Evaluate(point, center);
                colorScale(factor, col, addCol);
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
CKernel2D::VarCover(
    const Vector2D &center,
    COLOR &col,
    ScreenBuffer *ref,
    ScreenBuffer *dest,
    int totalSamples,
    int scaleSamples,
    float baseSize)
{
    float screenScale = floatMax(ref->getPixXSize(), ref->getPixYSize());
    //float screenFactor = ref->GetPixXSize() * ref->GetPixYSize();
    float B = baseSize * screenScale; // what about the 8 ??

    // Use optimal N (samples per pixel) dependency for fixed kernels
    // scaleSamples is normally total samples per pixel, while
    // totalSamples is the total number of samples for the CURRENT
    // number of samples per pixel
    float Bn = (float)(B * (std::pow((double) scaleSamples, (-1.5 / 5.0))));

    float h;

    // Now compute h for this sample

    // Reference estimated function
    COLOR fe = ref->getBiLinear(center.u, center.v);

    float avgFe = colorAverage(fe);
    float avgG = colorAverage(col);

    if ( avgFe > EPSILON ) {
        h = (float)(Bn * std::sqrt(avgG / avgFe));
        // printf("fe %f G %f, h = %f\n", avgFe, avgG, h/screenScale);
    } else {
        const float maxRatio = 20; // ???
        h = Bn * maxRatio * screenScale;

        printf("MaxRatio... h = %f\n", h / screenScale);
    }

    h = floatMax(1.0f * screenScale, h); // We want to cover at least one pixel...

    SetH(h);

    // h determined, now splat the fucker
    Cover(center, 1.0f / (float)totalSamples, col, dest);
}
