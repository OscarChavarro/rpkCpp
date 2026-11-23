package vsdk.toolkit.common.dataStructures;

import vsdk.toolkit.common.linealAlgebra.Vector3Df;

final class KDQuery<T> {
    Vector3Df point;
    int wanted;
    int found;
    T[] results;
    float[] distances;
    float maximumDistance;
    float squaredRadius;
    int excludeFlags;
}
