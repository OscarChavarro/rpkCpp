/**
Some bsdf component stuff.
*/

#ifndef BSDF_COMP__
#define BSDF_COMP__

#include "vsdk/toolkit/common/color/ColorRgb.h"

class BsdfComp {
  public:
    ColorRgb comp[BsdfComponentInfo::BSDF_COMPONENTS];

    BsdfComp();
    ColorRgb &operator[](int index);
    void Clear(const char flags);
    void Fill(const ColorRgb &col, const char flags);
    ColorRgb Sum(const char flags) const;
};

inline BsdfComp::BsdfComp():comp() {}

inline ColorRgb &
BsdfComp::operator[](int index) {
    return comp[index];
}

inline void
BsdfComp::Clear(const char flags = BsdfComponentInfo::BSDF_ALL_COMPONENTS) {
    for ( int i = 0; i < BsdfComponentInfo::BSDF_COMPONENTS; i++ ) {
        if ( flags & (BsdfComponentFlag::bsdfIndexToComp(i)) ) {
            comp[i] = ColorRgb(0.0, 0.0, 0.0);
        }
    }
}

inline void
BsdfComp::Fill(const ColorRgb &col, const char flags = BsdfComponentInfo::BSDF_ALL_COMPONENTS) {
    for ( int i = 0; i < BsdfComponentInfo::BSDF_COMPONENTS; i++ ) {
        if ( flags & (BsdfComponentFlag::bsdfIndexToComp(i)) ) {
            comp[i] = col;
        }
    }
}

inline ColorRgb
BsdfComp::Sum(const char flags = BsdfComponentInfo::BSDF_ALL_COMPONENTS) const {
    double red = 0.0;
    double green = 0.0;
    double blue = 0.0;

    for ( int i = 0; i < BsdfComponentInfo::BSDF_COMPONENTS; i++ ) {
        if ( flags & (BsdfComponentFlag::bsdfIndexToComp(i)) ) {
            red = red + comp[i].getR();
            green = green + comp[i].getG();
            blue = blue + comp[i].getB();
        }
    }

    return ColorRgb(red, green, blue);
}

#endif
