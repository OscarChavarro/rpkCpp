package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.color.ColorRgbMutable;
import vsdk.toolkit.common.dataStructures.CircularList;
import vsdk.toolkit.common.dataStructures.CircularListIterator;

public final class SparList extends CircularList<Spar> {
    public void
    handlePath(
        SparConfig config,
        BiPath path,
        ColorRgbMutable fRad,
        ColorRgbMutable fBpt)
    {
        CircularListIterator<Spar> iter = new CircularListIterator<>(this);
        Spar spar;
        ColorRgb col;

        fBpt.clear();
        fRad.clear();

        while ( (spar = iter.nextOnSequence()) != null ) {
            col = spar.handlePath(config, path);

            if ( spar == config.leSpar ) {
                fBpt.add(new ColorRgbMutable(col), fBpt);
            } else {
                fRad.add(new ColorRgbMutable(col), fRad);
            }
        }
    }
}
