/**
Some bsdf component stuff.
*/

#ifndef __BSDF_COMP__
#define __BSDF_COMP__

#include "common/VSDK.h"

class BsdfComp {
  public:
    ColorRgb comp[BsdfComponentInfo::BSDF_COMPONENTS];

    BsdfComp();
    ColorRgb &operator[](int index);
    operator ColorRgb *();
    void Clear(const char flags);
    void Fill(const ColorRgb col, const char flags);
    ColorRgb Sum(const char flags) const;
};

inline BsdfComp::BsdfComp():comp() {}

inline ColorRgb &
BsdfComp::operator[](int index) {
    return comp[index];
}

inline
BsdfComp::operator ColorRgb *() {
    return comp;
}

inline void
BsdfComp::Clear(const char flags = BsdfComponentInfo::BSDF_ALL_COMPONENTS) {
    for ( int i = 0; i < BsdfComponentInfo::BSDF_COMPONENTS; i++ ) {
        if ( flags & (BsdfComponentFlag::bsdfIndexToComp(i)) ) {
            comp[i].clear();
        }
    }
}

inline void
BsdfComp::Fill(const ColorRgb col, const char flags = BsdfComponentInfo::BSDF_ALL_COMPONENTS) {
    for ( int i = 0; i < BsdfComponentInfo::BSDF_COMPONENTS; i++ ) {
        if ( flags & (BsdfComponentFlag::bsdfIndexToComp(i)) ) {
            comp[i] = col;
        }
    }
}

inline ColorRgb
BsdfComp::Sum(const char flags = BsdfComponentInfo::BSDF_ALL_COMPONENTS) const {
    ColorRgb result;

    result.clear();

    for ( int i = 0; i < BsdfComponentInfo::BSDF_COMPONENTS; i++ ) {
        if ( flags & (BsdfComponentFlag::bsdfIndexToComp(i)) ) {
            result.add(result, comp[i]);
        }
    }

    return result;
}

#endif
