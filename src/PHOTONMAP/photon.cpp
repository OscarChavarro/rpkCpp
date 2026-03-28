#include "common/error.h"

#include "PHOTONMAP/photon.h"

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
    // Equation [ARVO1995b](6): rectangularToSphericalCoord projects onto the
    // plane orthogonal to the local Z-axis before recovering (phi, theta).
    coord->rectangularToSphericalCoord(&m_dir, &phi, &theta);

    // Compute r, s
    if ( flag == BRDF_DIFFUSE_COMPONENT ) {
        *s = phi / (2 * M_PI);
        double tmp = java::Math::cos(theta);
        *r = -tmp * tmp + 1;
    } else if ( flag == BRDF_GLOSSY_COMPONENT ) {
        *s = phi / (2 * M_PI);
        *r = java::Math::pow(java::Math::cos(theta), static_cast<double>(n) + 1.0);
    } else {
        logError("CPhoton::findRS", "Component %i not implemented yet", flag);
    }
}
