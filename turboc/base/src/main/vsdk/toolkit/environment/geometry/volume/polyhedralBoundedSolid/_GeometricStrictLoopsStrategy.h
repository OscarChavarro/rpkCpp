#ifndef __GMTRC_STRCT_LPS_STRTGY__
#define __GMTRC_STRCT_LPS_STRTGY__

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_PolyhedralBoundedSolidValidationStrategy.h"

/**
Checks loop geometry against the planar-loop expectations of the half-edge
representation from [MANT1988].10.2.1 and the planar polygon predicates of
chapter [MANT1988].13.
*/
class _GeometricStrictLoopsStrategy : public _PolyhedralBoundedSolidValidationStrategy {
public:
    virtual BOOLEAN validate(PolyhedralBoundedSolid* solid,
        const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
        String* msg);
};

#endif
