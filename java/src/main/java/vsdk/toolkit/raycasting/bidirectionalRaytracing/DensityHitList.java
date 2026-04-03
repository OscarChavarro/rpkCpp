package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.common.Error;

public class DensityHitList {
    protected static final int DHL_ARRAY_SIZE = 20;
    protected DensityHitArray first;
    protected DensityHitArray last;
    protected int numHits;
    protected int cacheLowerLimit;
    protected DensityHitArray cacheCurrent;

    public DensityHitList() {
        first = new DensityHitArray(DHL_ARRAY_SIZE);
        last = first;
        cacheCurrent = null;
        numHits = 0;
        cacheLowerLimit = 0;
    }

    public void add(DensityHit hit) {
        if ( !last.add(hit) ) {
            // New array needed

            last.setNext(new DensityHitArray(DHL_ARRAY_SIZE));
            last = last.getNext();

            last.add(hit); // Supposed not to fail
        }

        numHits++;
    }

    public int storedHits() {
        return numHits;
    }

    public DensityHit get(int i) {
        if ( i >= numHits ) {
            Error.fatal(-1, "DensityHitList::operator[]", "Index 'i' out of getBoundingBox");
        }

        if ( cacheCurrent == null || (i < cacheLowerLimit) ) {
            cacheCurrent = first;
            cacheLowerLimit = 0;
        }

        // Wanted point is beyond cacheCurrent
        while ( i >= cacheLowerLimit + DHL_ARRAY_SIZE ) {
            cacheCurrent = cacheCurrent.getNext();
            cacheLowerLimit += DHL_ARRAY_SIZE;
        }

        // Wanted point is in current cache block

        return cacheCurrent.get(i - cacheLowerLimit);
    }
}
