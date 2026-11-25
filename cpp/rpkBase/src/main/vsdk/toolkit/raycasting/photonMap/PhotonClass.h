#ifndef PHOTON_CLASS__
#define PHOTON_CLASS__

#include "vsdk/toolkit/common/linealAlgebra/CoordinateSystem.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/material/Xxdf.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonMapState.h"

// Non-compact photon representation
class Photon {
  protected:
    Vector3Df m_pos;  // Position
    ColorRgb m_power;  // power represented by this photon
    //  float m_dcWeight; // Weight for density control
    Vector3Df m_dir;  // Direction

  public:
    Photon() {};

    Photon(Vector3Df pos, const ColorRgb &power, const Vector3Df &dir)
            : m_pos(pos), m_power(power), m_dir(dir) {  }

    inline Vector3Df
    pos() const {
        return m_pos;
    }

    inline ColorRgb
    power() const {
        return m_power;
    }

    inline void
    addPower(const ColorRgb &col) {
        m_power = m_power.add(col);
    }

    inline Vector3Df
    dir() const {
        return m_dir;
    }

    const Vector3Df &getKDPosition() const { return m_pos; }

    // Importance sampling utility functions

    // Find the r,s values in a [0,1[^2 square corresponding to the photon
    void findRS(double *r, double *s, const CoordinateSystem *coord, char flag, float n) const;
};

#endif
