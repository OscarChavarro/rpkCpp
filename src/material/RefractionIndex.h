#ifndef __REFRACTION_INDEX__
#define __REFRACTION_INDEX__

class RefractionIndex {
public:
    float nr;
    float ni;

    float complexToGeometricRefractionIndex() const;
};

#endif
