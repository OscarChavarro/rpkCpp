#include "PHOTONMAP/photon.h"
#include "common/error.h"

void
CPhoton::findRS(
    double *r,
    double *s,
    const CoordinateSystem *coord,
    char flag, float n) const
{
    // Determine angles
    double phi;
    double theta;
    coord->rectangularToSphericalCoord(&m_dir, &phi, &theta);

    // Compute r, s
    if ( flag == BRDF_DIFFUSE_COMPONENT ) {
        *s = phi / (2 * M_PI);
        double tmp = java::Math::cos(theta);
        *r = -tmp * tmp + 1;
    } else if ( flag == BRDF_GLOSSY_COMPONENT ) {
        *s = phi / (2 * M_PI);
        *r = java::Math::pow(java::Math::cos(theta), (double)n + 1.0);
    } else {
        logError("CPhoton::findRS", "Component %i not implemented yet", flag);
    }
}
