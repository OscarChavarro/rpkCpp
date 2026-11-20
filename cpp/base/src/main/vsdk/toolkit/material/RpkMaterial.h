#ifndef RPK_MATERIAL__
#define RPK_MATERIAL__

#include "vsdk/toolkit/material/PhongBidirectionalScatteringDistributionFunction.h"
#include "vsdk/toolkit/material/PhongEmittanceDistributionFunction.h"
#include "vsdk/toolkit/environment/material/PhongMaterial.h"

/**
RenderPark material: the Phong EDF and BSDF objects the raytracers evaluate
and sample, plus the VITRAL PhongMaterial built from them at construction,
which answers the integrated reflectance and emittance the radiosity solvers
consume. The lobes cannot change after construction, so the two views stay
consistent.
*/
class RpkMaterial {
  private:
    PhongEmittanceDistributionFunction *edf; // Emittance distribution function
    PhongBidirectionalScatteringDistributionFunction *bsdf; // Reflection and transmission together
    bool sided; // True for 1-sided surface, false for 2-sided, see mgf docs
    char *name; // Material name
    const PhongMaterial *surfaceMaterial; // Owned; same lobes, VITRAL types

    static const PhongMaterial *buildSurfaceMaterial(
        const char *name,
        const PhongEmittanceDistributionFunction *edf,
        const PhongBidirectionalScatteringDistributionFunction *bsdf,
        bool sided);

  public:
    explicit RpkMaterial(
        const char *inName,
        PhongEmittanceDistributionFunction *edf,
        PhongBidirectionalScatteringDistributionFunction *bsdf,
        bool sided);
    virtual ~RpkMaterial();

    PhongEmittanceDistributionFunction *getEdf() const;
    PhongBidirectionalScatteringDistributionFunction *getBsdf() const;
    bool isSided() const;
    char *getName() const;
    const PhongMaterial *getSurfaceMaterial() const;
};

inline PhongEmittanceDistributionFunction *
RpkMaterial::getEdf() const {
    return edf;
}

inline PhongBidirectionalScatteringDistributionFunction *
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

inline const PhongMaterial *
RpkMaterial::getSurfaceMaterial() const {
    return surfaceMaterial;
}

#endif
