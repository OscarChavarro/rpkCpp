#ifndef __IMPORTON__
#define __IMPORTON__

#include "raycasting/photonMap/IrrPhoton.h"

// Importon: identical to IrrPhoton, but with some extra functions
class Importon : public IrrPhoton {
  public:
    inline void
    SetAll(float imp, float /*pot*/, float /*foot*/) {
        // Abuse m_power for importance estimates.
        // -- AT LEAST 3 COLOR components needed!  Watch out with compact photon repr.
        m_power = ColorRgb(imp, 0.0f, 0.0f);
    }

    inline void
    PSetAll(float imp, float /*pot*/, float /*foot*/) {
        // Abuse m_power for importance estimates.
        // -- AT LEAST 3 COLOR components needed!  Watch out with compact photon repr.
        m_irradiance = ColorRgb(imp, 0.0f, 0.0f);
    }

    Importon(
        Vector3D pos,
        float importance,
        float potential,
        float footprint,
        const Vector3D &dir)
    {
        m_pos = pos;
        m_dir = dir;

        SetAll(importance, potential, footprint);
    }

    inline float
    Importance() const {
        return m_power.getR();
    }

    inline float
    PImportance() const {
        return m_irradiance.getR();
    }
};

#endif
