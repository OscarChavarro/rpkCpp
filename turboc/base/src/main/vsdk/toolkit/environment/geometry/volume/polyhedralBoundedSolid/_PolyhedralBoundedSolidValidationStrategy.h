#ifndef __PBS_VALIDATION_STRATEGY__
#define __PBS_VALIDATION_STRATEGY__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidNumericPolicy.h"
class PolyhedralBoundedSolid;

/**
Strategy abstraction for checks that preserve the half-edge invariants of
[MANT1988].10 and the geometric validity conditions later used by chapters
[MANT1988].13 and [MANT1988].15.
*/
class _PolyhedralBoundedSolidValidationStrategy {
public:
    virtual ~_PolyhedralBoundedSolidValidationStrategy() {}

    /**
    Executes one validation criterion derived from the boundary-representation
    invariants discussed in chapters [MANT1988].10, [MANT1988].13, and
    [MANT1988].15.
    @param solid solid to validate
    @param numericContext numeric tolerance context
    @param msg receives failure details
    @return true when the criterion holds
    */
    virtual BOOLEAN validate(PolyhedralBoundedSolid* solid,
        const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
        String* msg) = 0;
};

#endif
