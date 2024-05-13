/**
Some bsdf component stuff.
*/

#ifndef __BSDF_COMP__
#define __BSDF_COMP__

class BsdfComp {
  public:
    ColorRgb comp[BSDF_COMPONENTS];

    BsdfComp():comp() {}

    inline ColorRgb &operator[](int index) {
        return comp[index];
    }

    // Conversion to COLOR *

    inline operator ColorRgb *() { return comp; }

    void Clear(const char flags = BSDF_ALL_COMPONENTS) {
        for ( int i = 0; i < BSDF_COMPONENTS; i++ ) {
            if ( flags & (BSDF_INDEX_TO_COMP(i)) ) {
                comp[i].clear();
            }
        }
    }

    void Fill(const ColorRgb col, const char flags = BSDF_ALL_COMPONENTS) {
        for ( int i = 0; i < BSDF_COMPONENTS; i++ ) {
            if ( flags & (BSDF_INDEX_TO_COMP(i)) ) {
                comp[i] = col;
            }
        }
    }

    ColorRgb Sum(const char flags = BSDF_ALL_COMPONENTS) {
        ColorRgb result;

        result.clear();

        for ( int i = 0; i < BSDF_COMPONENTS; i++ ) {
            if ( flags & (BSDF_INDEX_TO_COMP(i)) ) {
                result.add(result, comp[i]);
            }
        }

        return result;
    }
};

#endif
