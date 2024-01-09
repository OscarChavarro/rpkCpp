/* spherical.h: routine for sampling directions on a spherical triangle or 
 * quadrilateral using Arvo's technique published in SIGGRAPH '95 p 437 
 * and similar stuff. */

#ifndef _SPHERICAL_H_
#define _SPHERICAL_H_

#include <cstdio>

#include "skin/Patch.h"

class COORDSYS {
  public:
    Vector3D X, Y, Z;
};

/* creates a coordinate system on the patch P with Z direction along the normal */
extern void PatchCoordSys(Patch *P, COORDSYS *coord);

/* creates a coordinate system with the given UNIT direction vector as Z-axis */
extern void VectorCoordSys(Vector3D *Z, COORDSYS *coord);

/* Given a unit vector and a coordinate system, this routine computes the spherical
 * coordinates phi and theta of the vector with respect to the coordinate system */
extern void VectorToSphericalCoord(Vector3D *C, COORDSYS *coordsys, double *phi, double *theta);

extern void SphericalCoordToVector(COORDSYS *coordsys, double *phi, double *theta, Vector3D *C);

/* samples the hemisphere according to a cos_theta distribution */
extern Vector3D SampleHemisphereCosTheta(COORDSYS *coord, double xi_1, double xi_2, double *pdf_value);

/* samples the hemisphere according to a cos_theta ^ n  distribution */
extern Vector3D SampleHemisphereCosNTheta(COORDSYS *coord, double n, double xi_1, double xi_2, double *pdf_value);

/* defines a nice grid for stratified sampling */
extern void GetNrDivisions(int samples, int *divs1, int *divs2);

#endif

