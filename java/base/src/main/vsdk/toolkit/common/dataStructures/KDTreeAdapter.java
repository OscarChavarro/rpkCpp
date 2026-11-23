package vsdk.toolkit.common.dataStructures;

import vsdk.toolkit.common.linealAlgebra.Vector3Df;

public interface KDTreeAdapter<T> {
    Vector3Df getKDPosition(T value);
    T copy(T value);
}
