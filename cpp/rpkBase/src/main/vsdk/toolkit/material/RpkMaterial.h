#ifndef __RPK_MATERIAL__
#define __RPK_MATERIAL__

#include "vsdk/toolkit/environment/material/PhongLobe.h"
#include "vsdk/toolkit/environment/material/PhongMaterial.h"
#include "vsdk/toolkit/environment/material/Texture.h"
#include "vsdk/toolkit/material/PhongBidirectionalScatteringDistributionFunction.h"
#include "vsdk/toolkit/material/PhongEmittanceDistributionFunction.h"

/**
RenderPark material: the VITRAL PhongMaterial that holds its lobes and
answers every integrated and directional query (plan steps 2a and 2b), and
the two RenderPark views of it the raytracers call, with RenderPark's
argument types: the scattering view (null when the material has no BSDF)
and the emission view (null when it does not emit). Each material has its
own views, so a view pointer identifies its material (the raytracers use
them as media).

Ownership: the material owns its PhongMaterial (which owns the lobes), the
views and the texture.
*/
class RpkMaterial {
  private:
    char *name; // Material name
    bool sided; // True for 1-sided surface, false for 2-sided, see mgf docs
    Texture *texture; // Owned; shared read-only by the PhongMaterial
    const PhongMaterial *surfaceMaterial; // Owned
    PhongEmittanceDistributionFunction *edf; // Owned view, null when not emitting
    PhongBidirectionalScatteringDistributionFunction *bsdf; // Owned view, null without BSDF

  public:
    /**
    Adopts the lobes (nullptr for an absent one) and the texture. hasBsdf
    tells whether the material scatters at all (RenderPark's BSDF, possibly
    without reflection and transmission lobes); the refraction index is the
    transmission lobe's.
    */
    explicit RpkMaterial(
        const char *inName,
        const PhongLobe *emission,
        bool hasBsdf,
        const PhongLobe *reflection,
        const PhongLobe *transmission,
        Texture *inTexture,
        double refractionIndexReal,
        double refractionIndexImaginary,
        bool inSided);
    virtual ~RpkMaterial();
    RpkMaterial(const RpkMaterial &) = delete;
    RpkMaterial &operator=(const RpkMaterial &) = delete;

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
