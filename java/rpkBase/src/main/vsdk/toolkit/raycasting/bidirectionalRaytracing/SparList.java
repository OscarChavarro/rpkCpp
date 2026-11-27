package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.dataStructures.CircularList;
import vsdk.toolkit.common.dataStructures.CircularListIterator;

public final class SparList extends CircularList<Spar> {
    public SparContribution
    handlePath(
        SparConfig config,
        BiPath path)
    {
        CircularListIterator<Spar> iter = new CircularListIterator<>(this);
        Spar spar;
        ColorRgb radiance = new ColorRgb(0.0, 0.0, 0.0);
        ColorRgb bptRadiance = new ColorRgb(0.0, 0.0, 0.0);

        while ( (spar = iter.nextOnSequence()) != null ) {
            final ColorRgb contribution = spar.handlePath(config, path);

            if ( spar == config.leSpar ) {
                bptRadiance = bptRadiance.add(contribution);
            } else {
                radiance = radiance.add(contribution);
            }
        }
        return new SparContribution(radiance, bptRadiance);
    }
}
