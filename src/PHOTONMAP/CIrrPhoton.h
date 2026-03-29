#ifndef __C_IRR_PHOTON__
#define __C_IRR_PHOTON__

#include "PHOTONMAP/CPhoton.h"

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
        m_pos = photon.pos();
        m_power = photon.power();
        m_dir = photon.dir();
    }
};

#endif
