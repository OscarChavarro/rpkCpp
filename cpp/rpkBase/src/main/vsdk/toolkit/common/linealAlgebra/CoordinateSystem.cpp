#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/common/linealAlgebra/CoordinateSystem.h"

/**
Creates a coordinate system with the given UNIT direction vector as inZ-axis
*/
void
CoordinateSystem::setFromZAxis(const Vector3Dd *inZ) {
    Z = *inZ;

    // Equation [ARVO1995b](6): [x|y] = Normalize(x - (x·y) y).
    // X is a closed-form tangent orthogonal to Z (equivalent to [worldZ|Z], up to sign).
    double zz = java::Math::sqrt(1.0 - inZ->z() * inZ->z());

    if ( zz < VSDK::EPSILON ) {
        X = Vector3Dd(1.0, 0.0, 0.0);
    } else {
        X = Vector3Dd(inZ->y() / zz, -inZ->x() / zz, 0.0);
    }

    // Section [ARVO1995b].2: complete a local orthonormal frame to express spherical angles.
    Y = Z.crossProduct(X);
}

/**
Given a unit vector and a coordinate system, this routine computes the spherical
coordinates phi and theta of the vector with respect to the coordinate system
*/
void
CoordinateSystem::rectangularToSphericalCoord(const Vector3Dd *C, double *phi, double *theta) const {
    double z = C->dotProduct(Z);
    if ( z > 1.0 ) {
        z = 1.0;
    }
    // Sometimes numerical errors cause this
    if ( z < -1.0 ) {
        z = -1.0;
    }

    *theta = java::Math::acos(z);

    Vector3Dd c;

    // Equation [ARVO1995b](6): [C|Z] = Normalize(C - (C·Z) Z).
    c = (*C).add(Z.multiply(static_cast<double>(-z)));
    c = c.normalized(Numeric::EPSILON);
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
Vector3Dd
CoordinateSystem::sampleHemisphereCosTheta(double xi1, double xi2, double *probabilityDensityFunction) const {
    // Section [ARVO1995b].2: map (xi1, xi2) in [0,1]^2 to angular parameters on the sphere.
    double phi = 2.0 * static_cast<double>(M_PI) * static_cast<double>(xi1);
    double cos_phi = java::Math::cos(phi);
    double sin_phi = java::Math::sin(phi);
    double cos_theta = static_cast<double>(java::Math::sqrt(1.0 - xi2));
    double sin_theta = static_cast<double>(java::Math::sqrt(xi2));

    Vector3Dd dir;
    dir = X.multiply(cos_phi).add(Y.multiply(sin_phi));
    dir = dir.multiply(sin_theta).add(Z.multiply(cos_theta));

    *probabilityDensityFunction = cos_theta / M_PI;

    return dir;
}

/**
Samples the hemisphere according to a cos_theta ^ n distribution
*/
Vector3Dd
CoordinateSystem::sampleHemisphereCosNTheta(
    double n,
    double xi1,
    double xi2,
    double *probabilityDensityFunction) const
{
    // Section [ARVO1995b].2: same square-to-sphere mapping pattern with a different radial CDF.
    double phi = 2.0 * static_cast<double>(M_PI) * static_cast<double>(xi1);
    double cosPhi = java::Math::cos(phi);
    double sinPhi = java::Math::sin(phi);
    double cosTheta = static_cast<double>(java::Math::pow(xi2, 1.0 / (n + 1)));
    double sinTheta = static_cast<double>(java::Math::sqrt(1.0 - cosTheta * cosTheta));

    Vector3Dd dir;
    dir = X.multiply(cosPhi).add(Y.multiply(sinPhi));
    dir = dir.multiply(sinTheta).add(Z.multiply(cosTheta));
    *probabilityDensityFunction = (n + 1.0) * java::Math::pow(static_cast<double>(cosTheta), n) / (2.0 * M_PI);

    return dir;
}
