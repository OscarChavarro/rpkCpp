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
const float ScatteringFrame::NORMALIZATION_EPSILON = 1e-6F;

/** [ARVO1995b] equation 6: [x|z] = Normalize(x - (x.z) z), in closed form */
Vector3Df
ScatteringFrame::horizontalAxis(const Vector3Df &unitZ) {
    const float zz = java::Math::sqrt(1.0F - unitZ.z() * unitZ.z());
    if ( zz < DEGENERATE_AXIS ) {
        return Vector3Df(1.0F, 0.0F, 0.0F);
    }
    return Vector3Df(unitZ.y() / zz, -unitZ.x() / zz, 0.0F);
}

/** Y = Z x X completes the frame */
Vector3Df
ScatteringFrame::completingAxis(const Vector3Df &unitZ, const Vector3Df &unitX) {
    return Vector3Df(
        unitZ.y() * unitX.z() - unitZ.z() * unitX.y(),
        unitZ.z() * unitX.x() - unitZ.x() * unitX.z(),
        unitZ.x() * unitX.y() - unitZ.y() * unitX.x());
}

ScatteringFrame::ScatteringFrame(const Vector3Df &unitZ) :
    xAxis(horizontalAxis(unitZ)),
    yAxis(completingAxis(unitZ, xAxis)),
    zAxis(unitZ)
{
}

Vector3Df
ScatteringFrame::sampleCosine(const double xi1, const double xi2, double *density) const {
    // [ARVO1995b] section 2: (xi1, xi2) in [0,1]^2 mapped to azimuth and polar angle
    const float phi = 2.0F * static_cast<float>(M_PI) * static_cast<float>(xi1);
    const float cosPhi = java::Math::cos(phi);
    const float sinPhi = java::Math::sin(phi);
    const float cosTheta = static_cast<float>(java::Math::sqrt(1.0 - xi2));
    const float sinTheta = static_cast<float>(java::Math::sqrt(xi2));

    const Vector3Df planar = combine(cosPhi, xAxis, sinPhi, yAxis);
    *density = cosTheta / M_PI;
    return combine(sinTheta, planar, cosTheta, zAxis);
}

Vector3Df
ScatteringFrame::samplePhong(const double n, const double xi1, const double xi2, double *density) const {
    // [ARVO1995b] section 2: the same mapping with the radial distribution of cos^n
    const float phi = 2.0F * static_cast<float>(M_PI) * static_cast<float>(xi1);
    const float cosPhi = java::Math::cos(phi);
    const float sinPhi = java::Math::sin(phi);
    const float cosTheta = static_cast<float>(java::Math::pow(xi2, 1.0 / (n + 1)));
    const float sinTheta = static_cast<float>(java::Math::sqrt(1.0 - cosTheta * cosTheta));

    const Vector3Df planar = combine(cosPhi, xAxis, sinPhi, yAxis);
    *density = (n + 1.0) * java::Math::pow(static_cast<double>(cosTheta), n) / (2.0 * M_PI);
    return combine(sinTheta, planar, cosTheta, zAxis);
}

Vector3Df
ScatteringFrame::idealReflection(const Vector3Df &in, const Vector3Df &normal) {
    // The product is evaluated in single precision, then widened
    const double twice = 2 * dot(normal, in);
    const Vector3Df along = scale(static_cast<float>(twice), normal);
    const Vector3Df reflected(in.x() - along.x(), in.y() - along.y(), in.z() - along.z());
    return normalize(reflected, NORMALIZATION_EPSILON);
}

Vector3Df
ScatteringFrame::idealRefraction(
    const Vector3Df &in,
    const Vector3Df &normal,
    const float inIndex,
    const float outIndex,
    bool *totalInternalReflection)
{
    // [GLAS1989]: real part of the indices only
    const float ratio = inIndex / outIndex;
    const float cosIncident = -dot(in, normal);
    const float cosTransmitted2 = 1 + ratio * ratio * (cosIncident * cosIncident - 1);
    if ( cosTransmitted2 < 0 ) {
        *totalInternalReflection = true;
        return idealReflection(in, normal);
    }
    *totalInternalReflection = false;
    const float cosTransmitted = java::Math::sqrt(cosTransmitted2);
    const float normalScale = ratio * cosIncident - cosTransmitted;
    const Vector3Df scaled = scale(ratio, in);
    const Vector3Df refracted(
        scaled.x() + normalScale * normal.x(),
        scaled.y() + normalScale * normal.y(),
        scaled.z() + normalScale * normal.z());
    return normalize(refracted, NORMALIZATION_EPSILON);
}
