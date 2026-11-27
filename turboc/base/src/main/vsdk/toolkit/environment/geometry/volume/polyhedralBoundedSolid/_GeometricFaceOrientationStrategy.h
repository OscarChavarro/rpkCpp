#ifndef __GMTRC_FC_ORNTTN_STRTGY__
#define __GMTRC_FC_ORNTTN_STRTGY__

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_PolyhedralBoundedSolidValidationStrategy.h"

/**
Applies the face-orientation consistency expected from the 2-manifold
boundary model of [MANT1988].10.2.1. An inverted face plane normal breaks
both the visual rendering (back-facing triangles) and the face-equation
half-space test of [MANT1988].13.1 used downstream by the boolean pipeline.
*/
class _GeometricFaceOrientationStrategy : public _PolyhedralBoundedSolidValidationStrategy {
public:
    virtual BOOLEAN validate(PolyhedralBoundedSolid* solid,
        const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
        String* msg);
};

#endif
