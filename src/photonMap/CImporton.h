#ifndef __C_IMPORTON__
#define __C_IMPORTON__

#include "photonMap/CIrrPhoton.h"
// CImporton: identical to CIrrPhoton, but with some extra functions
class CImporton : public CIrrPhoton {
  public:
    inline void
    SetAll(float imp, float /*pot*/, float /*foot*/) {
        // Abuse m_power for importance estimates.
        // -- AT LEAST 3 COLOR components needed!  Watch out with compact photon repr.
        m_power.r = imp;
    }

    inline void
    PSetAll(float imp, float /*pot*/, float /*foot*/) {
        // Abuse m_power for importance estimates.
        // -- AT LEAST 3 COLOR components needed!  Watch out with compact photon repr.
        m_irradiance.r = imp;
    }

    CImporton(
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
        return m_power.r;
    }

    inline float
    PImportance() const {
        return m_irradiance.r;
    }
};

#endif
