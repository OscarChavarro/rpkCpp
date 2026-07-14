#ifndef DENSITY_HIT_ARRAY__
#define DENSITY_HIT_ARRAY__

#include "vsdk/toolkit/raycasting/bidirectionalRaytracing/DensityHit.h"

class DensityHitArray {
  private:
    DensityHit *hits;
    int maxHits;
    int numHits;
    DensityHitArray *next;

  public:
    explicit DensityHitArray(int paramMaxHits);
    ~DensityHitArray();
    bool add(const DensityHit &hit);
    DensityHit operator[](int i) const;
    DensityHitArray * getNext() const;
    void setNext(DensityHitArray *inNext);
};

inline DensityHitArray::DensityHitArray(int paramMaxHits) {
    numHits = 0;
    maxHits = paramMaxHits;
    hits = new DensityHit[paramMaxHits];
    next = nullptr;
}

inline DensityHitArray::~DensityHitArray() {
    delete[] hits;
}

inline bool
DensityHitArray::add(const DensityHit &hit) {
    if ( numHits < maxHits ) {
        hits[numHits++] = hit;
        return true;
    } else {
        return false;
    }
}

inline DensityHit
DensityHitArray::operator[](int i) const {
    return hits[i];
}

inline DensityHitArray *
DensityHitArray::getNext() const {
    return next;
}

inline void
DensityHitArray::setNext(DensityHitArray *inNext) {
    next = inNext;
}

#endif
