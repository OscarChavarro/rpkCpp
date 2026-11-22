#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/Math.h"
#include "vsdk/toolkit/common/linealAlgebra/Numeric.h"
#include "vsdk/toolkit/common/linealAlgebra/CoordinateSystem.h"

/**
Creates a coordinate system with the given UNIT direction vector as inZ-axis
*/
void
CoordinateSystem::setFromZAxis(const Vector3Df *inZ) {
    Z = *inZ;

    // Equation [ARVO1995b](6): [x|y] = Normalize(x - (x·y) y).
    // X is a closed-form tangent orthogonal to Z (equivalent to [worldZ|Z], up to sign).
    float zz = Math::sqrt(1.0f - inZ->z() * inZ->z());

    if ( zz < VSDK::EPSILON ) {
        X = Vector3Df(1.0, 0.0, 0.0);
    } else {
        X = Vector3Df(inZ->y() / zz, -inZ->x() / zz, 0.0);
    }

    // Section [ARVO1995b].2: complete a local orthonormal frame to express spherical angles.
    Y = Z.crossProduct(X);
}

/**
Given a unit vector and a coordinate system, this routine computes the spherical
coordinates phi and theta of the vector with respect to the coordinate system
*/
void
CoordinateSystem::rectangularToSphericalCoord(const Vector3Df *C, double *phi, double *theta) const {
    double z = C->dotProduct(Z);
    if ( z > 1.0 ) {
        z = 1.0;
    }
    // Sometimes numerical errors cause this
    if ( z < -1.0 ) {
        z = -1.0;
    }

    *theta = Math::acos(z);

    Vector3Df c;

    // Equation [ARVO1995b](6): [C|Z] = Normalize(C - (C·Z) Z).
    c = (*C).add(Z.multiply(((float)(-z))));
    c = c.normalized(Numeric::EPSILON_FLOAT);
    double x = c.dotProduct(X);
    double y = c.dotProduct(Y);

    if ( x > 1.0 ) {
        x = 1.0;
    }
    // Sometimes numerical errors cause this
    if ( x < -1.0 ) {
        x = -1.0;
    }
    *phi = Math::acos(x);
    if ( y < 0.0 ) {
        *phi = 2.0 * M_PI - *phi;
    }
}

/**
Samples the hemisphere according to a cos_theta distribution
*/
Vector3Df
CoordinateSystem::sampleHemisphereCosTheta(double xi1, double xi2, double *probabilityDensityFunction) const {
    // Section [ARVO1995b].2: map (xi1, xi2) in [0,1]^2 to angular parameters on the sphere.
    float phi = 2.0f * ((float)(M_PI)) * ((float)(xi1));
    float cos_phi = Math::cos(phi);
    float sin_phi = Math::sin(phi);
    float cos_theta = ((float)(Math::sqrt(1.0 - xi2)));
    float sin_theta = ((float)(Math::sqrt(xi2)));

    Vector3Df dir;
    dir = X.multiply(cos_phi).add(Y.multiply(sin_phi));
    dir = dir.multiply(sin_theta).add(Z.multiply(cos_theta));

    *probabilityDensityFunction = cos_theta / M_PI;

    return dir;
}

/**
Samples the hemisphere according to a cos_theta ^ n distribution
*/
Vector3Df
CoordinateSystem::sampleHemisphereCosNTheta(
    double n,
    double xi1,
    double xi2,
    double *probabilityDensityFunction) const
{
    // Section [ARVO1995b].2: same square-to-sphere mapping pattern with a different radial CDF.
    float phi = 2.0f * ((float)(M_PI)) * ((float)(xi1));
    float cosPhi = Math::cos(phi);
    float sinPhi = Math::sin(phi);
    float cosTheta = ((float)(Math::pow(xi2, 1.0 / (n + 1))));
    float sinTheta = ((float)(Math::sqrt(1.0 - cosTheta * cosTheta)));

    Vector3Df dir;
    dir = X.multiply(cosPhi).add(Y.multiply(sinPhi));
    dir = dir.multiply(sinTheta).add(Z.multiply(cosTheta));
    *probabilityDensityFunction = (n + 1.0) * Math::pow(((double)(cosTheta)), n) / (2.0 * M_PI);

    return dir;
}
