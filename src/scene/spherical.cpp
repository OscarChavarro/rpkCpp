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
    double zz;

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
vectorToSphericalCoord(Vector3D *C, COORDSYS *coordsys, double *phi, double *theta) {
    double x, y, z;
    Vector3D c;

    z = VECTORDOTPRODUCT(*C, coordsys->Z);
    if ( z > 1.0 ) {
        z = 1.0;
    }
    // Sometimes numerical errors cause this
    if ( z < -1.0 ) {
        z = -1.0;
    }

    *theta = acos(z);

    VECTORSUMSCALED(*C, -z, coordsys->Z, c);
    VECTORNORMALIZE(c);
    x = VECTORDOTPRODUCT(c, coordsys->X);
    y = VECTORDOTPRODUCT(c, coordsys->Y);

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
    COORDSYS *coordsys,
    double *phi,
    double *theta,
    Vector3D *C)
{
    double cos_phi;
    double sin_phi;
    double cos_theta;
    double sin_theta;
    Vector3D CP;

    cos_phi = cos(*phi);
    sin_phi = sin(*phi);
    cos_theta = cos(*theta);
    sin_theta = sin(*theta);

    VECTORCOMB2(cos_phi, coordsys->X, sin_phi, coordsys->Y, CP);
    VECTORCOMB2(cos_theta, coordsys->Z, sin_theta, CP, *C);
}

/**
Samples the hemisphere according to a cos_theta distribution
*/
Vector3D
sampleHemisphereCosTheta(COORDSYS *coord, double xi_1, double xi_2, double *pdf_value) {
    double phi;
    double cos_phi;
    double sin_phi;
    double cos_theta;
    double sin_theta;
    Vector3D dir;

    phi = 2.0 * M_PI * xi_1;
    cos_phi = cos(phi);
    sin_phi = sin(phi);
    cos_theta = sqrt(1.0 - xi_2);
    sin_theta = sqrt(xi_2);

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
    double phi;
    double cos_phi;
    double sin_phi;
    double cos_theta;
    double sin_theta;
    Vector3D dir;

    phi = 2. * M_PI * xi_1;
    cos_phi = cos(phi);
    sin_phi = sin(phi);
    cos_theta = pow(xi_2, 1.0 / (n + 1));
    sin_theta = sqrt(1.0 - cos_theta * cos_theta);

    VECTORCOMB2(cos_phi, coord->X,
                sin_phi, coord->Y,
                dir);
    VECTORCOMB2(sin_theta, dir,
                cos_theta, coord->Z,
                dir);

    *pdf_value = (n + 1.0) * pow(cos_theta, n) / (2.0 * M_PI);

    return dir;
}

/**
Makes a nice grid for stratified sampling
*/
void
getNumberOfDivisions(int samples, int *divs1, int *divs2) {
    if ( samples <= 0 ) {
        *divs1 = 0;
        *divs2 = 0;
        return;
    }

    *divs1 = (int) ceil(sqrt((double) samples));
    *divs2 = samples / (*divs1);
    while ((*divs1) * (*divs2) != samples && (*divs1) > 1 ) {
        (*divs1)--;
        *divs2 = samples / (*divs1);
    }
}
