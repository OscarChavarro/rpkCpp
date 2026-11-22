package vsdk.toolkit.environment.material;

import vsdk.toolkit.common.linealAlgebra.Vector3Df;

/**
Single precision vector arithmetic of the Phong scattering model, written
out component by component in the order RenderPark's Vector3D uses, so that
directions, cosines and densities are bit-identical to RenderPark's (the C++
namespace scatteringArithmetic of ScatteringFrame.h).
*/
public final class ScatteringArithmetic {
    private ScatteringArithmetic() {
    }

    public static float dot(Vector3Df a, Vector3Df b) {
        return a.x() * b.x() + a.y() * b.y() + a.z() * b.z();
    }

    public static Vector3Df scale(float s, Vector3Df v) {
        return new Vector3Df(s * v.x(), s * v.y(), s * v.z());
    }

    public static Vector3Df combine(float a, Vector3Df v, float b, Vector3Df w) {
        return new Vector3Df(a * v.x() + b * w.x(), a * v.y() + b * w.y(), a * v.z() + b * w.z());
    }

    /**
    v scaled by the inverse of its norm, left as it is when the norm is within
    epsilon of 0
    */
    public static Vector3Df normalize(Vector3Df v, float epsilon) {
        final float n = (float) Math.sqrt(v.x() * v.x() + v.y() * v.y() + v.z() * v.z());
        final float factor = n < -epsilon || n > epsilon ? 1.0F / n : 1.0F;
        return scale(factor, v);
    }
}
