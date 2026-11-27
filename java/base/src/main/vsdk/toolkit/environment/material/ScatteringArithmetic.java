package vsdk.toolkit.environment.material;

import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

/**
Double precision vector arithmetic of the Phong scattering model, written
out component by component in the order RenderPark's Vector3D uses, so that
directions, cosines and densities do not depend on Vector3Dd's methods.

cpp declares these functions in the namespace `scatteringArithmetic` of
`ScatteringFrame.h`; Java has no free functions, so they are the static
methods of this class.
*/
public final class ScatteringArithmetic {
    private ScatteringArithmetic()
    {
    }

    /** x * b.x + y * b.y + z * b.z, in that order */
    public static double dot(Vector3Dd a, Vector3Dd b)
    {
        return a.x() * b.x() + a.y() * b.y() + a.z() * b.z();
    }

    /** s * v */
    public static Vector3Dd scale(double s, Vector3Dd v)
    {
        return new Vector3Dd(s * v.x(), s * v.y(), s * v.z());
    }

    /** a * v + b * w */
    public static Vector3Dd combine(double a, Vector3Dd v, double b, Vector3Dd w)
    {
        return new Vector3Dd(a * v.x() + b * w.x(), a * v.y() + b * w.y(), a * v.z() + b * w.z());
    }

    /**
    v scaled by the inverse of its norm, left as it is when the norm is within
    epsilon of 0
    */
    public static Vector3Dd normalize(Vector3Dd v, double epsilon)
    {
        final double n = Math.sqrt(v.x() * v.x() + v.y() * v.y() + v.z() * v.z());
        final double factor = n < -epsilon || n > epsilon ? 1.0 / n : 1.0;
        return scale(factor, v);
    }
}
