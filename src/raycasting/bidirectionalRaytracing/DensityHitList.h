#ifndef __DENSITY_HIT_LIST__
#define __DENSITY_HIT_LIST__

#include "raycasting/bidirectionalRaytracing/DensityHitArray.h"

class DensityHitList {
  protected:
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
