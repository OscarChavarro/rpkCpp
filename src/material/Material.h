#ifndef __MATERIAL__
#define __MATERIAL__

#include "material/PhongBidirectionalScatteringDistributionFunction.h"
#include "material/PhongEmittanceDistributionFunction.h"

class Material {
  private:
    PhongEmittanceDistributionFunction *edf; // Emittance distribution function
    PhongBidirectionalScatteringDistributionFunction *bsdf; // Reflection and transmission together
    bool sided; // True for 1-sided surface, false for 2-sided, see mgf docs
    char *name; // Material name

  public:
    explicit Material(
        const char *inName,
        PhongEmittanceDistributionFunction *edf,
        PhongBidirectionalScatteringDistributionFunction *bsdf,
        bool sided);
    virtual ~Material();

    PhongEmittanceDistributionFunction *getEdf() const;
    PhongBidirectionalScatteringDistributionFunction *getBsdf() const;
    bool isSided() const;
    char *getName() const;
};

inline PhongEmittanceDistributionFunction *
Material::getEdf() const {
    return edf;
}

inline PhongBidirectionalScatteringDistributionFunction *
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
