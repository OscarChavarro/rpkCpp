/**
Density estimation on screen
*/

#ifndef __DENSITY_BUFFER__
#define __DENSITY_BUFFER__

#include "java/lang/Math.h"
#include "common/linealAlgebra/Vector2D.h"
#include "common/ColorRgb.h"
#include "render/ScreenBuffer.h"
#include "raycasting/raytracing/bidiroptions.h"

/**
Class CDensityBuffer : class for storing sample hits on screen
New samples are added with 'Add'. 'reconstruct' reconstructs
an approximation to the sampled function into a screen buffer
*/
class CDensityHit {
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

    CDensityHit(): m_x(), m_y() {}
    CDensityHit(float x, float y, ColorRgb col): m_x(), m_y() {
        init(x, y, col);
    }
};

class CDensityHitArray {
  protected:
    CDensityHit *hits;
    int maxHits;
    int numHits;

    CDensityHitArray *next;

  public:
    explicit CDensityHitArray(int paramMaxHits) {
        numHits = 0;
        maxHits = paramMaxHits;
        hits = new CDensityHit[paramMaxHits];
        next = nullptr;
    }

    ~CDensityHitArray() { delete[] hits; }

    bool add(const CDensityHit &hit) {
        if ( numHits < maxHits ) {
            hits[numHits++] = hit;
            return true;
        } else {
            return false;
        }
    }

    CDensityHit operator[](int i) const {
        return hits[i];
    }

    friend class CDensityHitList;
};

class CDensityHitList {
  protected:
    CDensityHitArray *m_first;
    CDensityHitArray *m_last;
    int m_numHits;

    int m_cacheLowerLimit;
    CDensityHitArray *m_cacheCurrent;

  public:
    CDensityHitList();
    ~CDensityHitList();
    void add(CDensityHit &hit);
    int storedHits() const { return m_numHits; }
    CDensityHit operator[](int i);
};

const int DHA_X_RES = 50;
const int DHA_Y_RES = 50;  // Subdivide image plane for efficient hit searches

class CDensityBuffer {
  protected:
    // A matching screen buffer is kept. This one will be filled in
    // by density estimation...
    ScreenBuffer *screenBuffer;
    BP_BASECONFIG *baseConfig;
    float xMinimum; // Copy from screenBuffer
    float xMaximum;
    float yMinimum;
    float yMaximum;
    CDensityHitList hitGrid[DHA_X_RES][DHA_Y_RES];

    inline int xIndex(float x) const {
        return java::Math::min(
            (int)(DHA_X_RES * (x - xMinimum) / (xMaximum - xMinimum)),
             DHA_X_RES - 1);
    }

    inline int yIndex(float y) const {
        return java::Math::min(
            (int)(DHA_Y_RES * (y - yMinimum) / (yMaximum - yMinimum)),
            DHA_Y_RES - 1);
    }

  public:
    CDensityBuffer(ScreenBuffer *screen, BP_BASECONFIG *paramBaseConfig);
    ~CDensityBuffer();
    void add(float x, float y, ColorRgb color);
    ScreenBuffer *reconstruct();
    ScreenBuffer *reconstructVariable(ScreenBuffer *dest, float baseSize = 4.0);
};

#endif
