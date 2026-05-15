#ifndef __MATERIAL__
#define __MATERIAL__

#include "common/VSDK.h"

#include "material/PhongBidirectionalScatteringDistributionFunction.h"
#include "material/PhongEmittanceDistributionFunction.h"

class Material {
  private:
    PhongEmitDistFunc *edf; // Emittance distribution function
    PhongBidirScattDistFunc *bsdf; // Reflection and transmission together
    bool sided; // True for 1-sided surface, false for 2-sided, see mgf docs
    char *name; // Material name

  public:
    explicit Material(
        const char *inName,
        PhongEmitDistFunc *edf,
        PhongBidirScattDistFunc *bsdf,
        bool sided);
    virtual ~Material();

    PhongEmitDistFunc *getEdf() const;
    PhongBidirScattDistFunc *getBsdf() const;
    bool isSided() const;
    char *getName() const;
};

inline PhongEmitDistFunc *
Material::getEdf() const {
    return edf;
}

inline PhongBidirScattDistFunc *
Material::getBsdf() const {
    return bsdf;
}

inline bool
Material::isSided() const {
    return sided;
}

inline char *
Material::getName() const {
    return name;
}

#endif
