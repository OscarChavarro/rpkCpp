#ifndef __GMTRC_STRCT_FC_INTRS_STRTG__
#define __GMTRC_STRCT_FC_INTRS_STRTG__

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_PolyhedralBoundedSolidValidationStrategy.h"

/**
Checks the non-self-intersection requirement imposed on boundary models in
[MANT1988].15.2, criterion 3, using the geometric intersection tools of
chapter [MANT1988].13.
*/
class _GeometricStrictFaceIntersectionsStrategy : public _PolyhedralBoundedSolidValidationStrategy {
public:
    virtual BOOLEAN validate(PolyhedralBoundedSolid* solid,
        const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
        String* msg);
};

#endif
