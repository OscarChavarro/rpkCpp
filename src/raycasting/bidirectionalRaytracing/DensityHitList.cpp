#include "common/RenderOptions.h"

#ifdef RAYTRACING_ENABLED

#include "raycasting/bidirectionalRaytracing/DensityHitList.h"
#include "common/error.h"

static const int DHL_ARRAY_SIZE = 20;

DensityHitList::DensityHitList(): cacheLowerLimit() {
    first = new DensityHitArray(DHL_ARRAY_SIZE);
    last = first;
    cacheCurrent = nullptr;
    numHits = 0;
}

DensityHitList::~DensityHitList() {
    DensityHitArray *tmpDA;

    while ( first != nullptr ) {
        tmpDA = first;
        first = first->getNext();
        delete tmpDA;
    }
}

DensityHit
DensityHitList::operator[](int i) {
    if ( i >= numHits ) {
        logFatal(-1, __FILE__ ":DensityHitList::operator[]", "Index 'i' out of getBoundingBox");
    }

    if ( !cacheCurrent || (i < cacheLowerLimit) ) {
        cacheCurrent = first;
        cacheLowerLimit = 0;
    }

    // Wanted point is beyond cacheCurrent
    while ( i >= cacheLowerLimit + DHL_ARRAY_SIZE ) {
        cacheCurrent = cacheCurrent->getNext();
        cacheLowerLimit += DHL_ARRAY_SIZE;
    }

    // Wanted point is in current cache block

    return ((*cacheCurrent)[i - cacheLowerLimit]);
}

void
DensityHitList::add(const DensityHit &hit) {
    if ( !last->add(hit) ) {
        // New array needed

        last->setNext(new DensityHitArray(DHL_ARRAY_SIZE));
        last = last->getNext();

        last->add(hit); // Supposed not to fail
    }

    numHits++;
}

#endif
