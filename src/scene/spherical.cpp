/*
routine to sample a spherical triangle or quadrilateral
using Arvo's technique published in SIGGRAPH '95 p 437.
*/

#include "common/mymath.h"
#include "scene/spherical.h"

/* creates a coordinate system on the patch P with Z direction along the normal */
void PatchCoordSys(PATCH *P, COORDSYS *coord) {
    coord->Z = P->normal;
    VECTORSUBTRACT(*P->vertex[1]->point, *P->vertex[0]->point, coord->X);
    VECTORNORMALIZE(coord->X);
    VECTORCROSSPRODUCT(coord->Z, coord->X, coord->Y);
}

/* creates a coordinate system with the given UNIT direction vector as Z-axis */
extern void VectorCoordSys(Vector3D *Z, COORDSYS *coord) {
    double zz;

    coord->Z = *Z;
    zz = sqrt(1 - Z->z * Z->z);

    if ( zz < EPSILON ) {
        coord->X.x = 1.;
        coord->X.y = 0.;
        coord->X.z = 0.;
    } else {
        coord->X.x = Z->y / zz;
        coord->X.y = -Z->x / zz;
        coord->X.z = 0.;
    }

    VECTORCROSSPRODUCT(coord->Z, coord->X, coord->Y);
}

/* J. Arvo, Stratified Sampling of Hemispherical Triangles, SIGGRAPH 95 p 437 */
Vector3Dd SampleSphericalTriangle(Vector3Dd *A, Vector3Dd *B, Vector3Dd *C,
                                  double Area, double alpha,
                                  double xi_1, double xi_2,
                                  double *pdf_value) {
    double Area1, s, t, u, v, cosalpha, sinalpha, cosc, q, q1, z, z1;
    Vector3Dd C1, P;

    cosc = VECTORDOTPRODUCT(*B, *A);

    cosalpha = cos(alpha);
    sinalpha = sin(alpha);

    Area1 = xi_1 * Area;

    s = sin(Area1 - alpha);
    t = cos(Area1 - alpha);

    u = t - cosalpha;
    v = s + sinalpha * cosc;

    q1 = (v * s + u * t) * sinalpha;
    if ( q1 > 1e-40 || q1 < -1e-40 ) {    /* avoid division by zero */
        q = ((v * t - u * s) * cosalpha - v) / q1;
    } else {    /* malformed spherical triangle: choose a random point C1 between A and C */
        q = 1. - xi_1 * (1. - VECTORDOTPRODUCT(*A, *C));
    }
    if ( q < -1. ) {
        q = -1.;
    }
    if ( q > 1. ) {
        q = 1.;
    }

    Vector3Dd cc(C->x, C->y, C->z);
    Vector3Dd aa(A->x, A->y, A->z);
    VECTORORTHOCOMP(cc, aa, C1);
    
    VECTORNORMALIZE(C1);
    q1 = sqrt(1. - q * q);
    VECTORSCALE(q1, C1, C1);
    VECTORSUMSCALED(C1, q, *A, C1);

    z = 1. - xi_2 * (1. - VECTORDOTPRODUCT(C1, *B));
    if ( z < -1. ) {
        z = -1.;
    }
    if ( z > 1. ) {
        z = 1.;
    }

    VECTORORTHOCOMP(C1, *B, P);
    VECTORNORMALIZE(P);
    z1 = sqrt(1. - z * z);
    VECTORSCALE(z1, P, P);
    VECTORSUMSCALED(P, z, *B, P);

    *pdf_value = 1.0 / Area;

    return P;
}

/* Girard's formula, see Arvo, "Irradiance Tensors", SIGGRAPH '95 */
void SphTriangleArea(Vector3Dd *A, Vector3Dd *B, Vector3Dd *C,
                     double *Area, double *alpha) {
    double beta, gamma, cosalpha, cosbeta, cosgamma, n1, n2, n3;
    Vector3Dd N1, N2, N3;

    VECTORCROSSPRODUCT(*C, *B, N1);
    n1 = VECTORNORM(N1);
    VECTORCROSSPRODUCT(*A, *C, N2);
    n2 = VECTORNORM(N2);
    VECTORCROSSPRODUCT(*B, *A, N3);
    n3 = VECTORNORM(N3);

    cosalpha = -VECTORDOTPRODUCT(N2, N3) / (n2 * n3);
    if ( cosalpha < -1. ) {
        cosalpha = -1.;
    }
    if ( cosalpha > 1. ) {
        cosalpha = 1.;
    }
    *alpha = acos(cosalpha);

    cosbeta = -VECTORDOTPRODUCT(N3, N1) / (n3 * n1);
    if ( cosbeta < -1. ) {
        cosbeta = -1.;
    }
    if ( cosbeta > 1. ) {
        cosbeta = 1.;
    }
    beta = acos(cosbeta);

    cosgamma = -VECTORDOTPRODUCT(N1, N2) / (n1 * n2);
    if ( cosgamma < -1. ) {
        cosgamma = -1.;
    }
    if ( cosgamma > 1. ) {
        cosgamma = 1.;
    }
    gamma = acos(cosgamma);

    *Area = *alpha + beta + gamma - M_PI;
}

/* Given a unit vector and a coordinate system, this routine computes the spherical
 * coordinates phi and theta of the vector with respect to the coordinate system */
void VectorToSphericalCoord(Vector3D *C, COORDSYS *coordsys, double *phi, double *theta) {
    double x, y, z;
    Vector3D c;

    z = VECTORDOTPRODUCT(*C, coordsys->Z);
    if ( z > 1.0 ) {
        z = 1.0;
    }  /* Sometimes numerical errors cause this */
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
    }  /* Sometimes numerical errors cause this */
    if ( x < -1.0 )
        x = -1.0;
    *phi = acos(x);
    if ( y < 0. )
        *phi = 2. * M_PI - *phi;
}

extern void SphericalCoordToVector(COORDSYS *coordsys, double *phi,
                                   double *theta, Vector3D *C) {
    double cos_phi, sin_phi, cos_theta, sin_theta;
    Vector3D CP;

    cos_phi = cos(*phi);
    sin_phi = sin(*phi);
    cos_theta = cos(*theta);
    sin_theta = sin(*theta);

    VECTORCOMB2(cos_phi, coordsys->X, sin_phi, coordsys->Y, CP);
    VECTORCOMB2(cos_theta, coordsys->Z, sin_theta, CP, *C);
}

Vector3D SampleHemisphereCosTheta(COORDSYS *coord, double xi_1, double xi_2, double *pdf_value) {
    double phi, cos_phi, sin_phi, cos_theta, sin_theta;
    Vector3D dir;

    phi = 2. * M_PI * xi_1;
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

Vector3D SampleHemisphereCosNTheta(COORDSYS *coord, double n, double xi_1, double xi_2, double *pdf_value) {
    double phi, cos_phi, sin_phi, cos_theta, sin_theta;
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

/***********************************************************/

/* makes a nice grid for stratified sampling */
void GetNrDivisions(int samples, int *divs1, int *divs2) {
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

