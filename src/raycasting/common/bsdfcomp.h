/**
Some bsdf component stuff.
*/

#ifndef __BSDFCOMP__
#define __BSDFCOMP__

class BsdfComp {
  public:
    COLOR comp[BSDF_COMPONENTS];

    inline COLOR &operator[](int index) {
        return comp[index];
    }

    // Conversion to COLOR *

    inline operator COLOR *() { return comp; }

    void Clear(const BSDF_FLAGS flags = BSDF_ALL_COMPONENTS) {
        for ( int i = 0; i < BSDF_COMPONENTS; i++ ) {
            if ( flags & (BSDF_INDEXTOCOMP(i)) ) {
                colorClear(comp[i]);
            }
        }
    }

    void Fill(const COLOR col, const BSDF_FLAGS flags = BSDF_ALL_COMPONENTS) {
        for ( int i = 0; i < BSDF_COMPONENTS; i++ ) {
            if ( flags & (BSDF_INDEXTOCOMP(i)) ) {
                comp[i] = col;
            }
        }
    }

    COLOR Sum(const BSDF_FLAGS flags = BSDF_ALL_COMPONENTS) {
        COLOR result;

        colorClear(result);

        for ( int i = 0; i < BSDF_COMPONENTS; i++ ) {
            if ( flags & (BSDF_INDEXTOCOMP(i))) {
                colorAdd(result, comp[i], result);
            }
        }

        return result;
    }
};

#endif
