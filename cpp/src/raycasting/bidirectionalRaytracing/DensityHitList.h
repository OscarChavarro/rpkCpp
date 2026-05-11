#ifndef DENSITY_HIT_LIST__
#define DENSITY_HIT_LIST__

#include "raycasting/bidirectionalRaytracing/DensityHitArray.h"

class DensityHitList {
  protected:
    static constexpr int DHL_ARRAY_SIZE = 20;
    DensityHitArray *first;
    DensityHitArray *last;
    int numHits;
    int cacheLowerLimit;
    DensityHitArray *cacheCurrent;

  public:
    DensityHitList();
    ~DensityHitList();
    void add(const DensityHit &hit);
    int storedHits() const;
    DensityHit operator[](int i);
};

inline int
DensityHitList::storedHits() const {
    return numHits;
}

#endif
