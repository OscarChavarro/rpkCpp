//= References:                                                             =
//= [ARVO1995b] Arvo, James. "Stratified Sampling of Spherical Triangles". =
//=     SIGGRAPH '95, 1995, pp. 437-438.                                    =
//= [GLAS1989] Glassner, Andrew S. (ed.). "An Introduction to Ray Tracing".=
//=     Academic Press, 1989.                                               =

#include <cmath>
#include "java/lang/Math.h"
#include "vsdk/toolkit/environment/material/ScatteringFrame.h"

using namespace scatteringArithmetic;

const double ScatteringFrame::DEGENERATE_AXIS = 1e-6;
const double ScatteringFrame::NORMALIZATION_EPSILON = 1e-6;

/** [ARVO1995b] equation 6: [x|z] = Normalize(x - (x.z) z), in closed form */
Vector3Dd
ScatteringFrame::horizontalAxis(const Vector3Dd &unitZ) {
    const double zz = java::Math::sqrt(1.0 - unitZ.z() * unitZ.z());
    if ( zz < DEGENERATE_AXIS ) {
        return Vector3Dd(1.0, 0.0, 0.0);
    }
    return Vector3Dd(unitZ.y() / zz, -unitZ.x() / zz, 0.0);
}

/** Y = Z x X completes the frame */
Vector3Dd
ScatteringFrame::completingAxis(const Vector3Dd &unitZ, const Vector3Dd &unitX) {
    return Vector3Dd(
        unitZ.y() * unitX.z() - unitZ.z() * unitX.y(),
        unitZ.z() * unitX.x() - unitZ.x() * unitX.z(),
        unitZ.x() * unitX.y() - unitZ.y() * unitX.x());
}

ScatteringFrame::ScatteringFrame(const Vector3Dd &unitZ) :
    x(horizontalAxis(unitZ)),
    y(completingAxis(unitZ, x)),
    z(unitZ)
{
}

Vector3Dd
ScatteringFrame::sampleCosine(const double xi1, const double xi2, double *density) const {
    // [ARVO1995b] section 2: (xi1, xi2) in [0,1]^2 mapped to azimuth and polar angle
    const double phi = 2.0 * static_cast<double>(M_PI) * static_cast<double>(xi1);
    const double cosPhi = java::Math::cos(phi);
    const double sinPhi = java::Math::sin(phi);
    const double cosTheta = static_cast<double>(java::Math::sqrt(1.0 - xi2));
    const double sinTheta = static_cast<double>(java::Math::sqrt(xi2));

    const Vector3Dd planar = combine(cosPhi, x, sinPhi, y);
    *density = cosTheta / M_PI;
    return combine(sinTheta, planar, cosTheta, z);
}

Vector3Dd
ScatteringFrame::samplePhong(const double n, const double xi1, const double xi2, double *density) const {
    // [ARVO1995b] section 2: the same mapping with the radial distribution of cos^n
    const double phi = 2.0 * static_cast<double>(M_PI) * static_cast<double>(xi1);
    const double cosPhi = java::Math::cos(phi);
    const double sinPhi = java::Math::sin(phi);
    const double cosTheta = static_cast<double>(java::Math::pow(xi2, 1.0 / (n + 1)));
    const double sinTheta = static_cast<double>(java::Math::sqrt(1.0 - cosTheta * cosTheta));

    const Vector3Dd planar = combine(cosPhi, x, sinPhi, y);
    *density = (n + 1.0) * java::Math::pow(static_cast<double>(cosTheta), n) / (2.0 * M_PI);
    return combine(sinTheta, planar, cosTheta, z);
}

Vector3Dd
ScatteringFrame::idealReflection(const Vector3Dd &in, const Vector3Dd &normal) {
    // The product is evaluated in single precision, then widened
    const double twice = 2 * dot(normal, in);
    const Vector3Dd along = scale(static_cast<double>(twice), normal);
    const Vector3Dd reflected(in.x() - along.x(), in.y() - along.y(), in.z() - along.z());
    return normalize(reflected, NORMALIZATION_EPSILON);
}

Vector3Dd
ScatteringFrame::idealRefraction(
    const Vector3Dd &in,
    const Vector3Dd &normal,
    const double inIndex,
    const double outIndex,
    bool *totalInternalReflection)
{
    // [GLAS1989]: real part of the indices only
    const double ratio = inIndex / outIndex;
    const double cosIncident = -dot(in, normal);
    const double cosTransmitted2 = 1 + ratio * ratio * (cosIncident * cosIncident - 1);
    if ( cosTransmitted2 < 0 ) {
        *totalInternalReflection = true;
        return idealReflection(in, normal);
    }
    *totalInternalReflection = false;
    const double cosTransmitted = java::Math::sqrt(cosTransmitted2);
    const double normalScale = ratio * cosIncident - cosTransmitted;
    const Vector3Dd scaled = scale(ratio, in);
    const Vector3Dd refracted(
        scaled.x() + normalScale * normal.x(),
        scaled.y() + normalScale * normal.y(),
        scaled.z() + normalScale * normal.z());
    return normalize(refracted, NORMALIZATION_EPSILON);
}
