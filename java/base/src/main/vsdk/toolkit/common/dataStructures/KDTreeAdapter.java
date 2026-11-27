package vsdk.toolkit.common.dataStructures;

import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

public interface KDTreeAdapter<T> {
    Vector3Dd getKDPosition(T value);
    T copy(T value);
}
