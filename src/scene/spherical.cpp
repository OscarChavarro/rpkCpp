/**
Routine to sample a spherical triangle or quadrilateral
using Arvo's technique published in SIGGRAPH '95 p 437
*/

#include "common/mymath.h"
#include "scene/spherical.h"

/**
Creates a coordinate system on the patch P with Z direction along the normal
*/
void
patchCoordSys(Patch *P, COORDSYS *coord) {
    coord->Z = P->normal;
    VECTORSUBTRACT(*P->vertex[1]->point, *P->vertex[0]->point, coord->X);
    VECTORNORMALIZE(coord->X);
    VECTORCROSSPRODUCT(coord->Z, coord->X, coord->Y);
}

/**
Creates a coordinate system with the given UNIT direction vector as Z-axis
*/
void
vectorCoordSys(Vector3D *Z, COORDSYS *coord) {
    float zz;

    coord->Z = *Z;
    zz = sqrt(1 - Z->z * Z->z);

    if ( zz < EPSILON ) {
        coord->X.x = 1.0;
        coord->X.y = 0.0;
        coord->X.z = 0.0;
    } else {
        coord->X.x = Z->y / zz;
        coord->X.y = -Z->x / zz;
        coord->X.z = 0.0;
    }

    VECTORCROSSPRODUCT(coord->Z, coord->X, coord->Y);
}

/**
Given a unit vector and a coordinate system, this routine computes the spherical
coordinates phi and theta of the vector with respect to the coordinate system
*/
void
vectorToSphericalCoord(Vector3D *C, COORDSYS *coordSys, double *phi, double *theta) {
    double x, y, z;
    Vector3D c;

    z = VECTORDOTPRODUCT(*C, coordSys->Z);
    if ( z > 1.0 ) {
        z = 1.0;
    }
    // Sometimes numerical errors cause this
    if ( z < -1.0 ) {
        z = -1.0;
    }

    *theta = acos(z);

    VECTORSUMSCALED(*C, -z, coordSys->Z, c);
    VECTORNORMALIZE(c);
    x = VECTORDOTPRODUCT(c, coordSys->X);
    y = VECTORDOTPRODUCT(c, coordSys->Y);

    if ( x > 1.0 ) {
        x = 1.0;
    }
    // Sometimes numerical errors cause this
    if ( x < -1.0 )
        x = -1.0;
    *phi = acos(x);
    if ( y < 0. )
        *phi = 2. * M_PI - *phi;
}

void
sphericalCoordToVector(
    COORDSYS *coordSys,
    const double *phi,
    const double *theta,
    Vector3D *C)
{
    Vector3D CP;

    float cos_phi = (float)std::cos(*phi);
    float sin_phi = (float)std::sin(*phi);
    float cos_theta = (float)std::cos(*theta);
    float sin_theta = (float)std::sin(*theta);

    VECTORCOMB2(cos_phi, coordSys->X, sin_phi, coordSys->Y, CP);
    VECTORCOMB2(cos_theta, coordSys->Z, sin_theta, CP, *C);
}

/**
Samples the hemisphere according to a cos_theta distribution
*/
Vector3D
sampleHemisphereCosTheta(COORDSYS *coord, double xi_1, double xi_2, double *pdf_value) {
    float phi;
    Vector3D dir;

    phi = 2.0f * (float)M_PI * (float)xi_1;
    float cos_phi = (float)std::cos(phi);
    float sin_phi = (float)std::sin(phi);
    float cos_theta = (float)std::sqrt(1.0 - xi_2);
    float sin_theta = (float)std::sqrt(xi_2);

    VECTORCOMB2(cos_phi, coord->X,
                sin_phi, coord->Y,
                dir);
    VECTORCOMB2(sin_theta, dir,
                cos_theta, coord->Z,
                dir);

    *pdf_value = cos_theta / M_PI;

    return dir;
}

/**
Samples the hemisphere according to a cos_theta ^ n  distribution
*/
Vector3D
sampleHemisphereCosNTheta(COORDSYS *coord, double n, double xi_1, double xi_2, double *pdf_value) {
    float phi;
    Vector3D dir;

    phi = 2.0f * (float)M_PI * (float)xi_1;
    float cos_phi = (float)std::cos(phi);
    float sin_phi = (float)std::sin(phi);
    float cos_theta = (float)std::pow(xi_2, 1.0 / (n + 1));
    float sin_theta = (float)std::sqrt(1.0 - cos_theta * cos_theta);

    VECTORCOMB2(cos_phi, coord->X,
                sin_phi, coord->Y,
                dir);
    VECTORCOMB2(sin_theta, dir,
                cos_theta, coord->Z,
                dir);

    *pdf_value = (n + 1.0) * pow(cos_theta, n) / (2.0 * M_PI);

    return dir;
}
