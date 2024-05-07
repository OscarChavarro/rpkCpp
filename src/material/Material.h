#ifndef __MATERIAL__
#define __MATERIAL__

#include "material/PhongEmittanceDistributionFunction.h"
#include "material/PhongBidirectionalScatteringDistributionFunction.h"

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

    inline PhongEmittanceDistributionFunction *
    getEdf() const {
        return edf;
    }

    inline PhongBidirectionalScatteringDistributionFunction *
    getBsdf() const {
        return bsdf;
    }

    inline bool
    isSided() const {
        return sided;
    }

    inline char *
    getName() const {
        return name;
    }
};

#endif
