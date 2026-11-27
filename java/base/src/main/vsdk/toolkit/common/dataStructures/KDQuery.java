package vsdk.toolkit.common.dataStructures;

import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

final class KDQuery<T> {
    Vector3Dd point;
    int wanted;
    int found;
    T[] results;
    double[] distances;
    double maximumDistance;
    double squaredRadius;
    int excludeFlags;
}
