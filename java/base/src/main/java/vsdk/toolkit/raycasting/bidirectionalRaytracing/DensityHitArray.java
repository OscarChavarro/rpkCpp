package vsdk.toolkit.raycasting.bidirectionalRaytracing;

public class DensityHitArray {
    private DensityHit[] hits;
    private int maxHits;
    private int numHits;
    private DensityHitArray next;

    public DensityHitArray(int paramMaxHits) {
        numHits = 0;
        maxHits = paramMaxHits;
        hits = new DensityHit[paramMaxHits];
        for ( int i = 0; i < paramMaxHits; i++ ) {
            hits[i] = new DensityHit();
        }
        next = null;
    }

    public boolean add(DensityHit hit) {
        if ( numHits < maxHits ) {
            hits[numHits] = new DensityHit(hit.m_x, hit.m_y, hit.color);
            numHits++;
            return true;
        } else {
            return false;
        }
    }

    public DensityHit get(int i) {
        return hits[i];
    }

    public DensityHitArray getNext() {
        return next;
    }

    public void setNext(DensityHitArray inNext) {
        next = inNext;
    }
}
