#ifndef __GEOMETRIC_STRICT_FACE_INTERSECTIONS_STRATEGY__
#define __GEOMETRIC_STRICT_FACE_INTERSECTIONS_STRATEGY__

#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_PolyhedralBoundedSolidValidationStrategy.h"

/**
Checks the non-self-intersection requirement imposed on boundary models in
[MANT1988].15.2, criterion 3, using the geometric intersection tools of
chapter [MANT1988].13.
*/
class _GeometricStrictFaceIntersectionsStrategy : public _PolyhedralBoundedSolidValidationStrategy {
public:
    virtual bool validate(PolyhedralBoundedSolid* solid,
        const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
        java::String* msg) override;
};

#endif
