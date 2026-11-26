#ifndef __GEOMETRIC_FACE_ORIENTATION_STRATEGY__
#define __GEOMETRIC_FACE_ORIENTATION_STRATEGY__

#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_PolyhedralBoundedSolidValidationStrategy.h"

/**
Applies the face-orientation consistency expected from the 2-manifold
boundary model of [MANT1988].10.2.1. An inverted face plane normal breaks
both the visual rendering (back-facing triangles) and the face-equation
half-space test of [MANT1988].13.1 used downstream by the boolean pipeline.
*/
class _GeometricFaceOrientationStrategy : public _PolyhedralBoundedSolidValidationStrategy {
public:
    virtual bool validate(PolyhedralBoundedSolid* solid,
        const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
        java::String* msg) override;
};

#endif
