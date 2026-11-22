//= References:                                                             =
//= [ARVO1995b] Arvo, James. "Stratified Sampling of Spherical Triangles". =
//=     SIGGRAPH '95, 1995, pp. 437-438.                                    =
//= [GLAS1989] Glassner, Andrew S. (ed.). "An Introduction to Ray Tracing".=
//=     Academic Press, 1989.                                               =

package vsdk.toolkit.environment.material;

import vsdk.toolkit.common.linealAlgebra.Vector3Df;

import static vsdk.toolkit.environment.material.ScatteringArithmetic.combine;
import static vsdk.toolkit.environment.material.ScatteringArithmetic.dot;
import static vsdk.toolkit.environment.material.ScatteringArithmetic.normalize;
import static vsdk.toolkit.environment.material.ScatteringArithmetic.scale;

/**
Orthonormal frame around a unit Z axis, with the hemisphere sampling of the
Phong scattering model ([ARVO1995b] section 2: a square of variates mapped
to angles on the sphere, in a frame aligned with the axis of the lobe), and
the ideal reflection and refraction directions it is centered on.

Direction convention: `in` is the propagation direction of the arriving
ray, so it points towards the surface (the opposite of PhongLobe's `in`,
which leaves it), `normal` leaves the surface on the side the ray comes
from, a returned direction leaves the surface. Densities are per
unit solid angle. Variates are supplied by the caller, who draws them.

Immutable value; all arithmetic in single precision except where noted, as
RenderPark computes it.
*/
public final class ScatteringFrame {
    /** Smallest |sin| of the Z axis' polar angle that still defines X from it */
    public static final double DEGENERATE_AXIS = 1e-6;
    /** Tolerance of the normalizations */
    public static final float NORMALIZATION_EPSILON = 1e-6F;

    private final Vector3Df xAxis;
    private final Vector3Df yAxis;
    private final Vector3Df zAxis;

    /**
    Frame whose Z axis is the given unit vector; X is horizontal
    (z = 0) when Z is not vertical, (1, 0, 0) when it is, and Y = Z x X.
    [ARVO1995b] equation 6 gives X up to its sign.
    */
    public ScatteringFrame(Vector3Df unitZ) {
        xAxis = horizontalAxis(unitZ);
        yAxis = completingAxis(unitZ, xAxis);
        zAxis = unitZ;
    }

    /** [ARVO1995b] equation 6: [x|z] = Normalize(x - (x.z) z), in closed form */
    private static Vector3Df horizontalAxis(Vector3Df unitZ) {
        final float zz = (float) Math.sqrt(1.0F - unitZ.z() * unitZ.z());
        if ( zz < DEGENERATE_AXIS ) {
            return new Vector3Df(1.0F, 0.0F, 0.0F);
        }
        return new Vector3Df(unitZ.y() / zz, -unitZ.x() / zz, 0.0F);
    }

    /** Y = Z x X completes the frame */
    private static Vector3Df completingAxis(Vector3Df unitZ, Vector3Df unitX) {
        return new Vector3Df(
            unitZ.y() * unitX.z() - unitZ.z() * unitX.y(),
            unitZ.z() * unitX.x() - unitZ.x() * unitX.z(),
            unitZ.x() * unitX.y() - unitZ.y() * unitX.x());
    }

    public Vector3Df getX() {
        return xAxis;
    }

    public Vector3Df getY() {
        return yAxis;
    }

    public Vector3Df getZ() {
        return zAxis;
    }

    /**
    Direction distributed as cos(theta) around Z, from variates xi1 (azimuth)
    and xi2 (polar); its density cos(theta) / pi goes to density[0]
    */
    public Vector3Df sampleCosine(double xi1, double xi2, double[] density) {
        // [ARVO1995b] section 2: (xi1, xi2) in [0,1]^2 mapped to azimuth and polar angle
        final float phi = 2.0F * (float) Math.PI * (float) xi1;
        final float cosPhi = (float) Math.cos(phi);
        final float sinPhi = (float) Math.sin(phi);
        final float cosTheta = (float) Math.sqrt(1.0 - xi2);
        final float sinTheta = (float) Math.sqrt(xi2);
        final Vector3Df planar = combine(cosPhi, xAxis, sinPhi, yAxis);
        density[0] = cosTheta / Math.PI;
        return combine(sinTheta, planar, cosTheta, zAxis);
    }

    /**
    Direction distributed as cos(theta)^n around Z; its density
    (n + 1) cos(theta)^n / (2 pi), computed in double precision, goes to
    density[0]
    */
    public Vector3Df samplePhong(double n, double xi1, double xi2, double[] density) {
        // [ARVO1995b] section 2: the same mapping with the radial distribution of cos^n
        final float phi = 2.0F * (float) Math.PI * (float) xi1;
        final float cosPhi = (float) Math.cos(phi);
        final float sinPhi = (float) Math.sin(phi);
        final float cosTheta = (float) Math.pow(xi2, 1.0 / (n + 1));
        final float sinTheta = (float) Math.sqrt(1.0 - cosTheta * cosTheta);
        final Vector3Df planar = combine(cosPhi, xAxis, sinPhi, yAxis);
        density[0] = (n + 1.0) * Math.pow((double) cosTheta, n) / (2.0 * Math.PI);
        return combine(sinTheta, planar, cosTheta, zAxis);
    }

    /** Mirror direction of `in` about `normal`, normalized */
    public static Vector3Df idealReflection(Vector3Df in, Vector3Df normal) {
        // The product is evaluated in single precision, then widened
        final double twice = 2 * dot(normal, in);
        final Vector3Df along = scale((float) twice, normal);
        final Vector3Df reflected = new Vector3Df(in.x() - along.x(), in.y() - along.y(), in.z() - along.z());
        return normalize(reflected, NORMALIZATION_EPSILON);
    }

    /**
    Refracted direction of `in` through the interface whose normal is
    `normal` ([GLAS1989]), with the real parts of the refraction indices on
    the side of `in` and beyond; on total internal reflection
    (totalInternalReflection[0] set) the mirror direction
    */
    public static Vector3Df idealRefraction(
        Vector3Df in,
        Vector3Df normal,
        float inIndex,
        float outIndex,
        boolean[] totalInternalReflection)
    {
        // [GLAS1989]: real part of the indices only
        final float ratio = inIndex / outIndex;
        final float cosIncident = -dot(in, normal);
        final float cosTransmitted2 = 1 + ratio * ratio * (cosIncident * cosIncident - 1);
        if ( cosTransmitted2 < 0 ) {
            totalInternalReflection[0] = true;
            return idealReflection(in, normal);
        }
        totalInternalReflection[0] = false;
        final float cosTransmitted = (float) Math.sqrt(cosTransmitted2);
        final float normalScale = ratio * cosIncident - cosTransmitted;
        final Vector3Df scaled = scale(ratio, in);
        final Vector3Df refracted = new Vector3Df(
            scaled.x() + normalScale * normal.x(),
            scaled.y() + normalScale * normal.y(),
            scaled.z() + normalScale * normal.z());
        return normalize(refracted, NORMALIZATION_EPSILON);
    }
}
