#ifndef __PHOTON_CLASS__
#define __PHOTON_CLASS__

#include "common/linealAlgebra/CoordinateSystem.h"
#include "common/color/ColorRgb.h"
#include "material/Xxdf.h"
#include "raycasting/photonMap/PhotonMapState.h"

// Non-compact photon representation
class Photon {
  protected:
    Vector3D m_pos;  // Position: 3 floats, MUST COME FIRST for kd tree storage
    ColorRgb m_power;  // power represented by this photon
    //  float m_dcWeight; // Weight for density control
    Vector3D m_dir;  // Direction

  public:
    Photon() {};

    Photon(Vector3D pos, const ColorRgb &power, const Vector3D &dir)
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

#endif
