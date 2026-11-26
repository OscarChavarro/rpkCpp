#ifndef __GEOMETRIC_STRICT_LOOPS_STRATEGY__
#define __GEOMETRIC_STRICT_LOOPS_STRATEGY__

#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_PolyhedralBoundedSolidValidationStrategy.h"

/**
Checks loop geometry against the planar-loop expectations of the half-edge
representation from [MANT1988].10.2.1 and the planar polygon predicates of
chapter [MANT1988].13.
*/
class _GeometricStrictLoopsStrategy : public _PolyhedralBoundedSolidValidationStrategy {
public:
    virtual bool validate(PolyhedralBoundedSolid* solid,
        const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
        java::String* msg) override;
};

#endif
