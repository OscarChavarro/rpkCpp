#ifndef __DENSITY_HIT_ARRAY__
#define __DENSITY_HIT_ARRAY__

#include "raycasting/bidirectionalRaytracing/DensityHit.h"

class DensityHitArray {
  private:
    DensityHit *hits;
    int maxHits;
    int numHits;
    DensityHitArray *next;

  public:
    explicit DensityHitArray(int paramMaxHits) {
        numHits = 0;
        maxHits = paramMaxHits;
        hits = new DensityHit[paramMaxHits];
        next = nullptr;
    }

    ~DensityHitArray() {
        delete[] hits;
    }

    bool add(const DensityHit &hit) {
        if ( numHits < maxHits ) {
            hits[numHits++] = hit;
            return true;
        } else {
            return false;
        }
    }

    DensityHit operator[](int i) const {
        return hits[i];
    }

    DensityHitArray *
    getNext() {
        return next;
    }

    void
    setNext(DensityHitArray *inNext) {
        next = inNext;
    }
};

#endif
