#ifndef IMPORTON__
#define IMPORTON__

#include "vsdk/toolkit/raycasting/photonMap/IrrPhoton.h"

// Importon: identical to IrrPhoton, but with some extra functions
class Importon : public IrrPhoton {
  public:
    inline void
    SetAll(float imp, float /*pot*/, float /*foot*/) {
        // Abuse m_power for importance estimates.
        // -- AT LEAST 3 COLOR components needed!  Watch out with compact photon repr.
        m_power.setR(imp);
    }

    inline void
    PSetAll(float imp, float /*pot*/, float /*foot*/) {
        // Abuse m_power for importance estimates.
        // -- AT LEAST 3 COLOR components needed!  Watch out with compact photon repr.
        m_irradiance.setR(imp);
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
        return static_cast<float>(m_power.getR());
    }

    inline float
    PImportance() const {
        return static_cast<float>(m_irradiance.getR());
    }
};

#endif
