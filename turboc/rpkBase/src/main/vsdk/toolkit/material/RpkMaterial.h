#ifndef __RPK_MATERIAL__
#define __RPK_MATERIAL__

#include "vsdk/toolkit/common/VSDK.h"

#include "vsdk/toolkit/material/PhongBidirectionalScatteringDistributionFunction.h"
#include "vsdk/toolkit/material/PhongEmittanceDistributionFunction.h"

class RpkMaterial {
  private:
    PhongEmitDistFunc *edf; // Emittance distribution function
    PhongBidirScattDistFunc *bsdf; // Reflection and transmission together
    bool sided; // True for 1-sided surface, false for 2-sided, see mgf docs
    char *name; // RpkMaterial name

  public:
    explicit RpkMaterial(
        const char *inName,
        PhongEmitDistFunc *edf,
        PhongBidirScattDistFunc *bsdf,
        bool sided);
    virtual ~RpkMaterial();

    PhongEmitDistFunc *getEdf() const;
    PhongBidirScattDistFunc *getBsdf() const;
    bool isSided() const;
    char *getName() const;
};

inline PhongEmitDistFunc *
RpkMaterial::getEdf() const {
    return edf;
}

inline PhongBidirScattDistFunc *
RpkMaterial::getBsdf() const {
    return bsdf;
}

inline bool
RpkMaterial::isSided() const {
    return sided;
}

inline char *
RpkMaterial::getName() const {
    return name;
}

#endif
