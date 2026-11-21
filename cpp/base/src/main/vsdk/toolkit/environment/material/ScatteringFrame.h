//= References:                                                             =
//= [ARVO1995b] Arvo, James. "Stratified Sampling of Spherical Triangles". =
//=     SIGGRAPH '95, 1995, pp. 437-438.                                    =
//= [GLAS1989] Glassner, Andrew S. (ed.). "An Introduction to Ray Tracing".=
//=     Academic Press, 1989.                                               =

#ifndef __SCATTERING_FRAME__
#define __SCATTERING_FRAME__

#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Df.h"

/**
Single precision vector arithmetic of the Phong scattering model, written
out component by component in the order RenderPark's Vector3D uses, so that
directions, cosines and densities are bit-identical to RenderPark's.
*/
namespace scatteringArithmetic {

/** x * b.x + y * b.y + z * b.z, in that order */
inline float
dot(const Vector3Df &a, const Vector3Df &b) {
    return a.x() * b.x() + a.y() * b.y() + a.z() * b.z();
}

/** s * v */
inline Vector3Df
scale(const float s, const Vector3Df &v) {
    return Vector3Df(s * v.x(), s * v.y(), s * v.z());
}

/** a * v + b * w */
inline Vector3Df
combine(const float a, const Vector3Df &v, const float b, const Vector3Df &w) {
    return Vector3Df(a * v.x() + b * w.x(), a * v.y() + b * w.y(), a * v.z() + b * w.z());
}

/**
v scaled by the inverse of its norm, left as it is when the norm is within
epsilon of 0. Inline, as RenderPark's Vector3D::normalize: under fast-math
the compiler orders the sum of squares from the context the vector comes
from, so an out-of-line copy rounds differently.
*/
inline Vector3Df
normalize(const Vector3Df &v, const float epsilon) {
    const float n = java::Math::sqrt(v.x() * v.x() + v.y() * v.y() + v.z() * v.z());
    const float factor = n < -epsilon || n > epsilon ? 1.0F / n : 1.0F;
    return scale(factor, v);
}

}

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
class ScatteringFrame {
  private:
    const Vector3Df xAxis;
    const Vector3Df yAxis;
    const Vector3Df zAxis;

    static Vector3Df horizontalAxis(const Vector3Df &unitZ);
    static Vector3Df completingAxis(const Vector3Df &unitZ, const Vector3Df &unitX);

  public:
    /** Smallest |sin| of the Z axis' polar angle that still defines X from it */
    static const double DEGENERATE_AXIS;

    /** Tolerance of the normalizations */
    static const float NORMALIZATION_EPSILON;

    /**
    Frame whose Z axis is the given unit vector; X is horizontal
    (z = 0) when Z is not vertical, (1, 0, 0) when it is, and Y = Z x X.
    [ARVO1995b] equation 6 gives X up to its sign.
    */
    explicit ScatteringFrame(const Vector3Df &unitZ);

    const Vector3Df &getX() const;
    const Vector3Df &getY() const;
    const Vector3Df &getZ() const;

    /**
    Direction distributed as cos(theta) around Z, from variates xi1 (azimuth)
    and xi2 (polar); its density cos(theta) / pi goes to *density
    */
    Vector3Df sampleCosine(double xi1, double xi2, double *density) const;

    /**
    Direction distributed as cos(theta)^n around Z; its density
    (n + 1) cos(theta)^n / (2 pi), computed in double precision, goes to
    *density
    */
    Vector3Df samplePhong(double n, double xi1, double xi2, double *density) const;

    /** Mirror direction of `in` about `normal`, normalized */
    static Vector3Df idealReflection(const Vector3Df &in, const Vector3Df &normal);

    /**
    Refracted direction of `in` through the interface whose normal is
    `normal` ([GLAS1989]), with the real parts of the refraction indices on
    the side of `in` and beyond; on total internal reflection
    *totalInternalReflection is set and the mirror direction is returned
    */
    static Vector3Df idealRefraction(
        const Vector3Df &in,
        const Vector3Df &normal,
        float inIndex,
        float outIndex,
        bool *totalInternalReflection);
};

inline const Vector3Df &
ScatteringFrame::getX() const {
    return xAxis;
}

inline const Vector3Df &
ScatteringFrame::getY() const {
    return yAxis;
}

inline const Vector3Df &
ScatteringFrame::getZ() const {
    return zAxis;
}

#endif
