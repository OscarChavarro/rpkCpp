/*
 * photon.C : photon implementation
 */

#include "PHOTONMAP/photon.h"
#include "common/error.h"

void CPhoton::FindRS(double *r, double *s, COORDSYS *coord,
                     BSDFFLAGS flag, float n) {
    double phi, theta;

    // Determine angles

    VectorToSphericalCoord(&m_dir, coord, &phi, &theta);

    // Compute r,s

    if ( flag == BRDF_DIFFUSE_COMPONENT ) {
        *s = phi / (2 * M_PI);
        double tmp = cos(theta);
        *r = -tmp * tmp + 1;
    } else if ( flag == BRDF_GLOSSY_COMPONENT ) {
        *s = phi / (2 * M_PI);
        *r = pow(cos(theta), n + 1);
    } else {
        Error("CPhoton::FindRS", "Component %i not implemented yet", flag);
    }
}
