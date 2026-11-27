#ifndef __TPLGCL_INTGRTY_STRTGY__
#define __TPLGCL_INTGRTY_STRTGY__

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_PolyhedralBoundedSolidValidationStrategy.h"

/**
Applies topological consistency checks for the half-edge data structure of
[MANT1988].10.2.1 and [MANT1988].10.2.2.
*/
class _TopologicalIntegrityStrategy : public _PolyhedralBoundedSolidValidationStrategy {
public:
    virtual BOOLEAN validate(PolyhedralBoundedSolid* solid,
        const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
        String* msg);
};

#endif
