/**
Density estimation on screen
*/

#ifndef __DENSITY_BUFFER__
#define __DENSITY_BUFFER__

#include "common/linealAlgebra/Vector2D.h"
#include "common/color.h"
#include "render/ScreenBuffer.h"
#include "raycasting/raytracing/bidiroptions.h"

/**
Class CDensityBuffer : class for storing sample hits on screen
New samples are added with 'Add'. 'Reconstruct' reconstructs
an approximation to the sampled function into a screen buffer
*/
class CDensityHit {
public:
    float m_x; // Screen/Polygon Coordinates
    float m_y;
    COLOR m_color;  // Estimate of the function, NOT divided by number of samples

    inline void Init(float x, float y, COLOR col) {
        m_x = x;
        m_y = y;
        m_color = col;
    }

    CDensityHit() {}
    CDensityHit(float x, float y, COLOR col) {
        Init(x, y, col);
    }
};

class CDensityHitArray {
protected:
    CDensityHit *m_hits;
    int m_maxHits;
    int m_numHits;

    CDensityHitArray *m_next;

public:
    CDensityHitArray(int maxHits) {
        m_numHits = 0;
        m_maxHits = maxHits;
        m_hits = new CDensityHit[maxHits];
        m_next = nullptr;
    }

    ~CDensityHitArray() { delete[] m_hits; }

    bool Add(CDensityHit &hit) {
        if ( m_numHits < m_maxHits ) {
            m_hits[m_numHits++] = hit;
            return true;
        } else {
            return false;
        }
    }

    CDensityHit operator[](int i) { return m_hits[i]; }

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

    void Add(CDensityHit &hit);

    int StoredHits() { return m_numHits; }

    CDensityHit operator[](int i);
};


const int DHA_XRES = 50;
const int DHA_YRES = 50;  // Subdivide imageplane for efficient hit searches


class CDensityBuffer {
protected:
    // A matching screen buffer is kept. This one will be filled in
    // by density estimation...
    ScreenBuffer *m_screen;
    BP_BASECONFIG *m_bcfg;
    float m_xmin, m_xmax, m_ymin, m_ymax; // Copy from m_screen...
    CDensityHitList m_hitGrid[DHA_XRES][DHA_YRES];

    inline int XIndex(float x) {
        return (floatMin((int) (DHA_XRES * (x - m_xmin) / (m_xmax - m_xmin)),
                         DHA_XRES - 1));
    }

    inline int YIndex(float y) {
        return (floatMin((int) (DHA_YRES * (y - m_ymin) / (m_ymax - m_ymin)),
                         DHA_YRES - 1));
    }


public:
    CDensityBuffer(ScreenBuffer *screen, BP_BASECONFIG *bcfg);

    ~CDensityBuffer();

    // Add a hit
    void Add(float x, float y, COLOR col, float pdf, float w = 1.0);

    // Reconstruct the internal screen buffer using constant kernel width
    ScreenBuffer *Reconstruct();

    ScreenBuffer *ReconstructVariable(ScreenBuffer *dest, float baseSize = 4.0);
};

#endif
