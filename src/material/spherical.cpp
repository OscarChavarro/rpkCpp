/**
Routine to sample a spherical triangle or quadrilateral
using Arvo's technique published in SIGGRAPH '95 p 437
*/

#include "common/mymath.h"
#include "material/spherical.h"

/**
Creates a coordinate system with the given UNIT direction vector as Z-axis
*/
void
vectorCoordSys(const Vector3D *Z, CoordSys *coord) {
    float zz;

    coord->Z = *Z;
    zz = std::sqrt(1 - Z->z * Z->z);

    if ( zz < EPSILON ) {
        coord->X.x = 1.0;
        coord->X.y = 0.0;
        coord->X.z = 0.0;
    } else {
        coord->X.x = Z->y / zz;
        coord->X.y = -Z->x / zz;
        coord->X.z = 0.0;
    }

    vectorCrossProduct(coord->Z, coord->X, coord->Y);
}

/**
Given a unit vector and a coordinate system, this routine computes the spherical
coordinates phi and theta of the vector with respect to the coordinate system
*/
void
vectorToSphericalCoord(const Vector3D *C, const CoordSys *coordSys, double *phi, double *theta) {
    double x;
    double y;
    double z;
    Vector3D c;

    z = vectorDotProduct(*C, coordSys->Z);
    if ( z > 1.0 ) {
        z = 1.0;
    }
    // Sometimes numerical errors cause this
    if ( z < -1.0 ) {
        z = -1.0;
    }

    *theta = std::acos(z);

    vectorSumScaled(*C, -z, coordSys->Z, c);
    vectorNormalize(c);
    x = vectorDotProduct(c, coordSys->X);
    y = vectorDotProduct(c, coordSys->Y);

    if ( x > 1.0 ) {
        x = 1.0;
    }
    // Sometimes numerical errors cause this
    if ( x < -1.0 )
        x = -1.0;
    *phi = std::acos(x);
    if ( y < 0.0 )
        *phi = 2.0 * M_PI - *phi;
}

/**
Samples the hemisphere according to a cos_theta distribution
*/
Vector3D
sampleHemisphereCosTheta(const CoordSys *coord, double xi_1, double xi_2, double *probabilityDensityFunction) {
    float phi;
    Vector3D dir;

    phi = 2.0f * (float)M_PI * (float)xi_1;
    float cos_phi = std::cos(phi);
    float sin_phi = std::sin(phi);
    float cos_theta = (float)std::sqrt(1.0 - xi_2);
    float sin_theta = (float)std::sqrt(xi_2);

    vectorComb2(cos_phi, coord->X,
                sin_phi, coord->Y,
                dir);
    vectorComb2(sin_theta, dir,
                cos_theta, coord->Z,
                dir);

    *probabilityDensityFunction = cos_theta / M_PI;

    return dir;
}

/**
Samples the hemisphere according to a cos_theta ^ n distribution
*/
Vector3D
sampleHemisphereCosNTheta(const CoordSys *coord, double n, double xi_1, double xi_2, double *probabilityDensityFunction) {
    Vector3D dir;

    float phi = 2.0f * (float)M_PI * (float)xi_1;
    float cosPhi = std::cos(phi);
    float sinPhi = std::sin(phi);
    float cosTheta = (float)std::pow(xi_2, 1.0 / (n + 1));
    float sinTheta = (float)std::sqrt(1.0 - cosTheta * cosTheta);

    vectorComb2(cosPhi, coord->X, sinPhi, coord->Y, dir);
    vectorComb2(sinTheta, dir, cosTheta, coord->Z, dir);
    *probabilityDensityFunction = (n + 1.0) * std::pow(cosTheta, n) / (2.0 * M_PI);

    return dir;
}
