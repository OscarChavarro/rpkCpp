#ifndef __POLYHEDRAL_BOUNDED_SOLID_VALIDATION_STRATEGY__
#define __POLYHEDRAL_BOUNDED_SOLID_VALIDATION_STRATEGY__

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
    virtual bool validate(PolyhedralBoundedSolid* solid,
        const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
        java::String* msg) = 0;
};

#endif
