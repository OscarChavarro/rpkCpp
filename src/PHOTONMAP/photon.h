/**
Data structure for individual photons
*/

#ifndef __PHOTON__
#define __PHOTON__

#include <cstring>

#include "common/ColorRgb.h"
#include "common/linealAlgebra/CoordinateSystem.h"
#include "PHOTONMAP/pmapoptions.h"
#include "material/xxdf.h"

// KD tree flags (currently not used)
const short DIRECT_LIGHT_PHOTON = 0x10;
const short CAUSTIC_LIGHT_PHOTON = 0x20; // Lower 4 bits reserved for kd tree
// This type of photon should not be included in the importance sampling
const short NO_IMPSAMP_PHOTON = DIRECT_LIGHT_PHOTON | CAUSTIC_LIGHT_PHOTON;


// Non-compact photon representation

class CPhoton {
  protected:
    Vector3D m_pos;  // Position: 3 floats, MUST COME FIRST for kd tree storage
    ColorRgb m_power;  // power represented by this photon
    //  float m_dcWeight; // Weight for density control
    Vector3D m_dir;  // Direction

  public:
    CPhoton() {};

    CPhoton(Vector3D pos, const ColorRgb &power, const Vector3D &dir)
            : m_pos(pos), m_power(power), m_dir(dir) {  }

    inline Vector3D
    pos() const {
        return m_pos;
    }

    inline ColorRgb
    power() const {
        return m_power;
    }

    inline void
    addPower(ColorRgb col) {
        m_power.add(m_power, col);
    }

    inline Vector3D
    dir() const {
        return m_dir;
    }

    // Importance sampling utility functions

    // Find the r,s values in a [0,1[^2 square corresponding to the photon
    void findRS(double *r, double *s, const CoordinateSystem *coord, char flag, float n) const;
};


// CIrrPhoton: photon with extra irradiance info
class CIrrPhoton : public CPhoton {
  public:
    Vector3D m_normal;
    ColorRgb m_irradiance;

    inline Vector3D Normal() const { return m_normal; }

    inline void setNormal(const Vector3D &normal) { m_normal = normal; }

    inline void SetIrradiance(const ColorRgb &irr) { m_irradiance = irr; }

    inline void
    copy(const CPhoton &photon) {
        // Dangerous ??
        memcpy((char *) this, &photon, sizeof(CPhoton));
    }
};

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
