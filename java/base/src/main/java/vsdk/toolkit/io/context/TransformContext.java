package vsdk.toolkit.io.context;

import vsdk.toolkit.common.linealAlgebra.Matrix4x4d;

public class TransformContext {
    public Matrix4x4d transformMatrix;
    public double scaleFactor;

    public TransformContext() {
        transformMatrix = new Matrix4x4d();
        scaleFactor = 0.0;
    }
}
