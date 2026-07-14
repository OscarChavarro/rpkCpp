#ifndef PHOTON_CLASS__
#define PHOTON_CLASS__

#include "vsdk/toolkit/common/linealAlgebra/CoordinateSystem.h"
#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "vsdk/toolkit/material/Xxdf.h"
#include "vsdk/toolkit/raycasting/photonMap/PhotonMapState.h"

// Non-compact photon representation
class Photon {
  protected:
    Vector3D m_pos;  // Position: 3 floats, MUST COME FIRST for kd tree storage
    ColorRgbMutable m_power;  // power represented by this photon
    //  float m_dcWeight; // Weight for density control
    Vector3D m_dir;  // Direction

  public:
    Photon() {};

    Photon(Vector3D pos, const ColorRgbMutable &power, const Vector3D &dir)
            : m_pos(pos), m_power(power), m_dir(dir) {  }

    inline Vector3D
    pos() const {
        return m_pos;
    }

    inline ColorRgbMutable
    power() const {
        return m_power;
    }

    inline void
    addPower(ColorRgbMutable col) {
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

#endif
