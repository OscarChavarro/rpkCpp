/**
Some bsdf component stuff.
*/

#ifndef BSDF_COMP__
#define BSDF_COMP__

class BsdfComp {
  public:
    ColorRgbMutable comp[BsdfComponentInfo::BSDF_COMPONENTS];

    BsdfComp();
    ColorRgbMutable &operator[](int index);
    operator ColorRgbMutable *();
    void Clear(const char flags);
    void Fill(const ColorRgbMutable col, const char flags);
    ColorRgbMutable Sum(const char flags) const;
};

inline BsdfComp::BsdfComp():comp() {}

inline ColorRgbMutable &
BsdfComp::operator[](int index) {
    return comp[index];
}

inline
BsdfComp::operator ColorRgbMutable *() {
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
BsdfComp::Fill(const ColorRgbMutable col, const char flags = BsdfComponentInfo::BSDF_ALL_COMPONENTS) {
    for ( int i = 0; i < BsdfComponentInfo::BSDF_COMPONENTS; i++ ) {
        if ( flags & (BsdfComponentFlag::bsdfIndexToComp(i)) ) {
            comp[i] = col;
        }
    }
}

inline ColorRgbMutable
BsdfComp::Sum(const char flags = BsdfComponentInfo::BSDF_ALL_COMPONENTS) const {
    ColorRgbMutable result;

    result.clear();

    for ( int i = 0; i < BsdfComponentInfo::BSDF_COMPONENTS; i++ ) {
        if ( flags & (BsdfComponentFlag::bsdfIndexToComp(i)) ) {
            result.add(result, comp[i]);
        }
    }

    return result;
}

#endif
