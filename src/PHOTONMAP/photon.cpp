#include "PHOTONMAP/photon.h"
#include "common/error.h"

void CPhoton::FindRS(double *r, double *s, CoordinateSystem *coord,
                     BSDF_FLAGS flag, float n) {
    double phi;
    double theta;

    // Determine angles
    coord->rectangularToSphericalCoord(&m_dir, &phi, &theta);

    // Compute r,s

    if ( flag == BRDF_DIFFUSE_COMPONENT ) {
        *s = phi / (2 * M_PI);
        double tmp = java::Math::cos(theta);
        *r = -tmp * tmp + 1;
    } else if ( flag == BRDF_GLOSSY_COMPONENT ) {
        *s = phi / (2 * M_PI);
        *r = java::Math::pow(java::Math::cos(theta), (double)n + 1.0);
    } else {
        logError("CPhoton::FindRS", "Component %i not implemented yet", flag);
    }
}
