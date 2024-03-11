#include "PHOTONMAP/photon.h"
#include "common/error.h"

void CPhoton::FindRS(double *r, double *s, CoordSys *coord,
                     BSDF_FLAGS flag, float n) {
    double phi;
    double theta;

    // Determine angles

    vectorToSphericalCoord(&m_dir, coord, &phi, &theta);

    // Compute r,s

    if ( flag == BRDF_DIFFUSE_COMPONENT ) {
        *s = phi / (2 * M_PI);
        double tmp = std::cos(theta);
        *r = -tmp * tmp + 1;
    } else if ( flag == BRDF_GLOSSY_COMPONENT ) {
        *s = phi / (2 * M_PI);
        *r = std::pow(std::cos(theta), n + 1);
    } else {
        logError("CPhoton::FindRS", "Component %i not implemented yet", flag);
    }
}
