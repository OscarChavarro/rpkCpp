#ifndef __TOPOLOGICAL_INTEGRITY_STRATEGY__
#define __TOPOLOGICAL_INTEGRITY_STRATEGY__

#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_PolyhedralBoundedSolidValidationStrategy.h"

/**
Applies topological consistency checks for the half-edge data structure of
[MANT1988].10.2.1 and [MANT1988].10.2.2.
*/
class _TopologicalIntegrityStrategy : public _PolyhedralBoundedSolidValidationStrategy {
public:
    virtual bool validate(PolyhedralBoundedSolid* solid,
        const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
        java::String* msg) override;
};

#endif
