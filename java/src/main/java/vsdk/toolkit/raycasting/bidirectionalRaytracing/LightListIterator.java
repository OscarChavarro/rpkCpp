package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.common.dataStructures.CircularListIterator;
import vsdk.toolkit.skin.Patch;

public class LightListIterator {
    private CircularListIterator<LightInfo> iterator;

    public LightListIterator(LightList list) {
        iterator = new CircularListIterator<>(list.entries());
    }

    public Patch First(LightList list) {
        iterator.init(list.entries());

        LightInfo li = iterator.nextOnSequence();
        if ( li != null ) {
            return li.light;
        } else {
            return null;
        }
    }

    public Patch Next() {
        LightInfo li = iterator.nextOnSequence();
        if ( li != null ) {
            return li.light;
        } else {
            return null;
        }
    }
}
