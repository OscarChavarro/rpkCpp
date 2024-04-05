#include "common/error.h"
#include "raycasting/raytracing/densitybuffer.h"
#include "raycasting/raytracing/densitykernel.h"

const int DHL_ARRAY_SIZE = 20;

CDensityHitList::CDensityHitList(): m_cacheLowerLimit() {
    m_first = new CDensityHitArray(DHL_ARRAY_SIZE);
    m_last = m_first;
    m_cacheCurrent = nullptr;

    m_numHits = 0;
}

CDensityHitList::~CDensityHitList() {
    CDensityHitArray *tmpDA;

    while ( m_first ) {
        tmpDA = m_first;
        m_first = m_first->next;
        delete tmpDA;
    }
}

CDensityHit CDensityHitList::operator[](int i) {
    if ( i >= m_numHits ) {
        logFatal(-1, __FILE__ ":CDensityHitList::operator[]", "Index 'i' out of getBoundingBox");
    }

    if ( !m_cacheCurrent || (i < m_cacheLowerLimit) ) {
        m_cacheCurrent = m_first;
        m_cacheLowerLimit = 0;
    }

    // Wanted point is beyond m_cacheCurrent
    while ( i >= m_cacheLowerLimit + DHL_ARRAY_SIZE ) {
        m_cacheCurrent = m_cacheCurrent->next;
        m_cacheLowerLimit += DHL_ARRAY_SIZE;
    }

    // Wanted point is in current cache block

    return ((*m_cacheCurrent)[i - m_cacheLowerLimit]);
}

void CDensityHitList::add(CDensityHit &hit) {
    if ( !m_last->Add(hit) ) {
        // New array needed

        m_last->next = new CDensityHitArray(DHL_ARRAY_SIZE);
        m_last = m_last->next;

        m_last->Add(hit); // Supposed not to fail
    }

    m_numHits++;
}



// CDensityBuffer implementation

CDensityBuffer::CDensityBuffer(ScreenBuffer *screen, BP_BASECONFIG *paramBaseConfig) {
    screenBuffer = screen;
    baseConfig = paramBaseConfig;

    xMinimum = screenBuffer->getScreenXMin();
    xMaximum = screenBuffer->getScreenXMax();
    yMinimum = screenBuffer->getScreenYMin();
    yMaximum = screenBuffer->getScreenYMax();

    printf("Density Buffer :\nXmin %f, Ymin %f, Xmax %f, Ymax %f\n",
           xMinimum, yMinimum, xMaximum, yMaximum);

}

CDensityBuffer::~CDensityBuffer() {
}

/**
Add a hit
*/
void
CDensityBuffer::add(float x, float y, ColorRgb color) {
    float factor = screenBuffer->getPixXSize() * screenBuffer->getPixYSize()
                   * (float) baseConfig->totalSamples;
    ColorRgb tmpCol;

    if ( color.average() > EPSILON ) {
        tmpCol.scaledCopy(factor, color); // Undo part of flux to rad factor

        CDensityHit hit(x, y, tmpCol);

        hitGrid[xIndex(x)][yIndex(y)].add(hit);
    }
}

/**
Reconstruct the internal screen buffer using constant kernel width
*/
ScreenBuffer *
CDensityBuffer::reconstruct() {
    // For all samples -> compute pixel coverage

    // Kernel size. Now spread over 3 pixels
    float h = 8.0f * floatMax(screenBuffer->getPixXSize(), screenBuffer->getPixYSize())
              / std::sqrt((float)baseConfig->samplesPerPixel);

    printf("h = %f\n", h);

    screenBuffer->scaleRadiance(0.0); // Hack!

    int i;
    int j;
    int k;
    int maxK;
    CDensityHit hit;
    CKernel2D kernel;
    Vector2D center;

    kernel.SetH(h);

    for ( i = 0; i < DHA_X_RES; i++ ) {
        for ( j = 0; j < DHA_Y_RES; j++ ) {
            maxK = hitGrid[i][j].storedHits();

            for ( k = 0; k < maxK; k++ ) {
                hit = (hitGrid[i][j])[k];

                center.u = hit.m_x;
                center.v = hit.m_y;

                kernel.Cover(center, 1.0f / (float)baseConfig->totalSamples, hit.color, screenBuffer);
            }
        }
    }

    return screenBuffer;
}


ScreenBuffer *
CDensityBuffer::reconstructVariable(ScreenBuffer *dest, float baseSize) {
    // For all samples -> compute pixel coverage

    // Base Kernel size. Now spread over a number of pixels

    dest->scaleRadiance(0.0); // Hack!

    int i;
    int j;
    int k;
    int maxK;
    CDensityHit hit;
    CKernel2D kernel;
    Vector2D center;

    for ( i = 0; i < DHA_X_RES; i++ ) {
        for ( j = 0; j < DHA_Y_RES; j++ ) {
            maxK = hitGrid[i][j].storedHits();

            for ( k = 0; k < maxK; k++ ) {
                hit = (hitGrid[i][j])[k];

                center.u = hit.m_x;
                center.v = hit.m_y;

                kernel.varCover(center, hit.color, screenBuffer, dest, (int) baseConfig->totalSamples,
                                baseConfig->samplesPerPixel, baseSize);
            }
        }
    }

    return dest;
}
