#include "common/logging/Logger.h"
#include "raycasting/photonMap/PhotonClass.h"
void
Photon::findRS(
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
        double tmp = Math::cos(theta);
        *r = -tmp * tmp + 1;
    } else if ( flag == BRDF_GLOSSY_COMPONENT ) {
        *s = phi / (2 * M_PI);
        *r = Math::pow(Math::cos(theta), ((double)(n)) + 1.0);
    } else {
        Logger::error("Photon::findRS", "Component %i not implemented yet", flag);
    }
}
