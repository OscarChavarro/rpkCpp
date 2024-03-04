/**
Some bsdf component stuff.
*/

#ifndef _BSDFCOMP_H_
#define _BSDFCOMP_H_

class CBsdfComp {
  public:

    // Only storage is this !
    COLOR comp[BSDFCOMPONENTS];

    // Methods

    // Default constructor/destructor

    // Element access

    inline COLOR &operator[](int index) {
        return comp[index];
    }

    // Conversion to COLOR *

    inline operator COLOR *() { return comp; }

    void Clear(const BSDFFLAGS flags = BSDF_ALL_COMPONENTS) {
        for ( int i = 0; i < BSDFCOMPONENTS; i++ ) {
            if ( flags & (BSDF_INDEXTOCOMP(i)) ) {
                colorClear(comp[i]);
            }
        }
    }

    void Fill(const COLOR col, const BSDFFLAGS flags = BSDF_ALL_COMPONENTS) {
        for ( int i = 0; i < BSDFCOMPONENTS; i++ ) {
            if ( flags & (BSDF_INDEXTOCOMP(i)) ) {
                comp[i] = col;
            }
        }
    }

    COLOR Sum(const BSDFFLAGS flags = BSDF_ALL_COMPONENTS) {
        COLOR result;

        colorClear(result);

        for ( int i = 0; i < BSDFCOMPONENTS; i++ ) {
            if ( flags & (BSDF_INDEXTOCOMP(i))) {
                colorAdd(result, comp[i], result);
            }
        }

        return result;
    }
};

#endif
