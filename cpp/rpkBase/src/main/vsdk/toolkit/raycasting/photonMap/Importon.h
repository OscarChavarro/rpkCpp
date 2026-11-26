#ifndef __IMPORTON__
#define __IMPORTON__

#include "vsdk/toolkit/raycasting/photonMap/IrrPhoton.h"

// Importon: identical to IrrPhoton, but with some extra functions
class Importon : public IrrPhoton {
  public:
    inline void
    SetAll(double imp, double /*pot*/, double /*foot*/) {
        // Abuse m_power for importance estimates.
        // -- AT LEAST 3 COLOR components needed!  Watch out with compact photon repr.
        m_power = ColorRgb(imp, imp, imp);
    }

    inline void
    PSetAll(double imp, double /*pot*/, double /*foot*/) {
        // Abuse m_power for importance estimates.
        // -- AT LEAST 3 COLOR components needed!  Watch out with compact photon repr.
        m_irradiance = ColorRgb(imp, imp, imp);
    }

    Importon(
        Vector3Dd pos,
        double importance,
        double potential,
        double footprint,
        const Vector3Dd &dir)
    {
        m_pos = pos;
        m_dir = dir;

        SetAll(importance, potential, footprint);
    }

    inline double
    Importance() const {
        return static_cast<double>(m_power.getR());
    }

    inline double
    PImportance() const {
        return static_cast<double>(m_irradiance.getR());
    }
};

#endif
