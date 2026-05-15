#ifndef __IRR_PHOTON__
#define __IRR_PHOTON__

#include "raycasting/photonMap/PhotonClass.h"
// IrrPhoton: photon with extra irradiance info
class IrrPhoton : public Photon {
  public:
    Vector3D m_normal;
    ColorRgb m_irradiance;

    inline Vector3D Normal() const { return m_normal; }

    inline void setNormal(const Vector3D &normal) { m_normal = normal; }

    inline void SetIrradiance(const ColorRgb &irr) { m_irradiance = irr; }

    inline void
    copy(const Photon &photon) {
        m_pos = photon.pos();
        m_power = photon.power();
        m_dir = photon.dir();
    }
};

#endif
