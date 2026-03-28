#include "java/lang/Math.h"
#include "common/linealAlgebra/Numeric.h"
#include "common/linealAlgebra/CoordinateSystem.h"

/**
Creates a coordinate system with the given UNIT direction vector as inZ-axis
*/
void
CoordinateSystem::setFromZAxis(const Vector3D *inZ) {
    Z = *inZ;

    // Equation [ARVO1995b](6): [x|y] = Normalize(x - (x·y) y).
    // X is a closed-form tangent orthogonal to Z (equivalent to [worldZ|Z], up to sign).
    float zz = java::Math::sqrt(1.0f - inZ->z * inZ->z);

    if ( zz < Numeric::EPSILON ) {
        X.x = 1.0;
        X.y = 0.0;
        X.z = 0.0;
    } else {
        X.x = inZ->y / zz;
        X.y = -inZ->x / zz;
        X.z = 0.0;
    }

    // Section [ARVO1995b].2: complete a local orthonormal frame to express spherical angles.
    Y.crossProduct(Z, X);
}

/**
Given a unit vector and a coordinate system, this routine computes the spherical
coordinates phi and theta of the vector with respect to the coordinate system
*/
void
CoordinateSystem::rectangularToSphericalCoord(const Vector3D *C, double *phi, double *theta) const {
    double z = C->dotProduct(Z);
    if ( z > 1.0 ) {
        z = 1.0;
    }
    // Sometimes numerical errors cause this
    if ( z < -1.0 ) {
        z = -1.0;
    }

    *theta = java::Math::acos(z);

    Vector3D c;

    // Equation [ARVO1995b](6): [C|Z] = Normalize(C - (C·Z) Z).
    c.sumScaled(*C, -z, Z);
    c.normalize(Numeric::EPSILON_FLOAT);
    double x = c.dotProduct(X);
    double y = c.dotProduct(Y);

    if ( x > 1.0 ) {
        x = 1.0;
    }
    // Sometimes numerical errors cause this
    if ( x < -1.0 ) {
        x = -1.0;
    }
    *phi = java::Math::acos(x);
    if ( y < 0.0 ) {
        *phi = 2.0 * M_PI - *phi;
    }
}

/**
Samples the hemisphere according to a cos_theta distribution
*/
Vector3D
CoordinateSystem::sampleHemisphereCosTheta(double xi1, double xi2, double *probabilityDensityFunction) const {
    // Section [ARVO1995b].2: map (xi1, xi2) in [0,1]^2 to angular parameters on the sphere.
    float phi = 2.0f * static_cast<float>(M_PI) * static_cast<float>(xi1);
    float cos_phi = java::Math::cos(phi);
    float sin_phi = java::Math::sin(phi);
    float cos_theta = static_cast<float>(java::Math::sqrt(1.0 - xi2));
    float sin_theta = static_cast<float>(java::Math::sqrt(xi2));

    Vector3D dir;
    dir.combine(cos_phi, X, sin_phi, Y);
    dir.combine(sin_theta, dir, cos_theta, Z);

    *probabilityDensityFunction = cos_theta / M_PI;

    return dir;
}

/**
Samples the hemisphere according to a cos_theta ^ n distribution
*/
Vector3D
CoordinateSystem::sampleHemisphereCosNTheta(
    double n,
    double xi1,
    double xi2,
    double *probabilityDensityFunction) const
{
    // Section [ARVO1995b].2: same square-to-sphere mapping pattern with a different radial CDF.
    float phi = 2.0f * static_cast<float>(M_PI) * static_cast<float>(xi1);
    float cosPhi = java::Math::cos(phi);
    float sinPhi = java::Math::sin(phi);
    float cosTheta = static_cast<float>(java::Math::pow(xi2, 1.0 / (n + 1)));
    float sinTheta = static_cast<float>(java::Math::sqrt(1.0 - cosTheta * cosTheta));

    Vector3D dir;
    dir.combine(cosPhi, X, sinPhi, Y);
    dir.combine(sinTheta, dir, cosTheta, Z);
    *probabilityDensityFunction = (n + 1.0) * java::Math::pow(static_cast<double>(cosTheta), n) / (2.0 * M_PI);

    return dir;
}
