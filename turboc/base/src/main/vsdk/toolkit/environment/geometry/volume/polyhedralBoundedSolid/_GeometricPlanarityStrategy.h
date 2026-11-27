#ifndef __GMTRC_PLNRTY_STRTGY__
#define __GMTRC_PLNRTY_STRTGY__

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_PolyhedralBoundedSolidValidationStrategy.h"

/**
Applies the planar-face consistency expected from the half-edge face model in
[MANT1988].10.2.1 and from the face-equation discussion of [MANT1988].13.1.
*/
class _GeometricPlanarityStrategy : public _PolyhedralBoundedSolidValidationStrategy {
public:
    virtual BOOLEAN validate(PolyhedralBoundedSolid* solid,
        const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
        String* msg);
};

#endif
