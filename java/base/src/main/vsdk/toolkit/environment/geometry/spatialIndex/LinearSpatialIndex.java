package vsdk.toolkit.environment.geometry.spatialIndex;

import vsdk.toolkit.environment.geometry.element.Ray;

/**
Reference `SpatialIndex`: no pruning at all. Every primitive is tested, in
increasing id order, so its answer is the one every conservative index must
agree with on visibility; with a tie rule that prefers the lowest id it is
also the closest-hit answer of brute force.
*/
public final class LinearSpatialIndex extends SpatialIndex {
    private final int numberOfPrimitives;

    public LinearSpatialIndex(int inNumberOfPrimitives) {
        numberOfPrimitives = inNumberOfPrimitives > 0 ? inNumberOfPrimitives : 0;
    }

    @Override
    public int getNumberOfPrimitives() {
        return numberOfPrimitives;
    }

    @Override
    public int getWorkspaceSize() {
        return 0;
    }

    @Override
    public SpatialQueryResult query(
        Ray ray,
        double minimumT,
        double maximumT,
        SpatialQueryMode mode,
        PrimitiveRayTest test,
        SpatialIndexWorkspace workspace) {
        SpatialQueryResult result = null;
        double bestT = maximumT;
        long tests = 0;
        final double[] hitT = workspace.getHitTScratch();
        for ( int primitive = 0; primitive < numberOfPrimitives; primitive++ ) {
            tests++;
            if ( test.test(primitive, minimumT, bestT, hitT) ) {
                bestT = hitT[0];
                result = new SpatialQueryResult(primitive, hitT[0]);
                if ( mode == SpatialQueryMode.ANY_HIT ) {
                    break;
                }
            }
        }
        workspace.addPrimitiveTests(tests);
        return result != null ? result : new SpatialQueryResult();
    }
}
