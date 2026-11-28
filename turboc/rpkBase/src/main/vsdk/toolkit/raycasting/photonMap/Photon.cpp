#include "java/lang/Math.h"
#include "vsdk/toolkit/skin/Numeric.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/common/logging/LoggerFormat.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonClass.h"
void
Photon::findRS(
    double *r,
    double *s,
    const CoordinateSystem *coord,
    char flag, double n) const
{
    // Determine angles
    double phi;
    double theta;
    // Equation [ARVO1995b](6): rectangularToSphericalCoord projects onto the
    // plane orthogonal to the local Z-axis before recovering (phi, theta).
    coord->rectangularToSphericalCoord(&m_dir, &phi, &theta, Numeric::EPSILON);

    // Compute r, s
    if ( flag == BRDF_DIFFUSE_COMPONENT ) {
        *s = phi / (2 * M_PI);
        double tmp = Math::cos(theta);
        *r = -tmp * tmp + 1;
    } else if ( flag == BRDF_GLOSSY_COMPONENT ) {
        *s = phi / (2 * M_PI);
        *r = Math::pow(Math::cos(theta), ((double)(n)) + 1.0);
    } else {
        Logger::reportMessage("Photon", Logger::ERROR, "Photon::findRS", LoggerFormat::format("Component %i not implemented yet", flag));
    }
}
