package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.dataStructures.CircularList;
import vsdk.toolkit.common.dataStructures.CircularListIterator;

public final class SparList extends CircularList<Spar> {
    public void
    handlePath(
        SparConfig config,
        BiPath path,
        ColorRgb fRad,
        ColorRgb fBpt)
    {
        CircularListIterator<Spar> iter = new CircularListIterator<>(this);
        Spar spar;
        ColorRgb col;

        fBpt.clear();
        fRad.clear();

        while ( (spar = iter.nextOnSequence()) != null ) {
            col = spar.handlePath(config, path);

            if ( spar == config.leSpar ) {
                fBpt.add(col, fBpt);
            } else {
                fRad.add(col, fRad);
            }
        }
    }
}
