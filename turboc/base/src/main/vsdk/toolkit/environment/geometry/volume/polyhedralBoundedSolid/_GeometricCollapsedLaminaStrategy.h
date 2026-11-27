//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#ifndef __GMTRC_CLLPSD_LMN_STRTGY__
#define __GMTRC_CLLPSD_LMN_STRTGY__

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_PolyhedralBoundedSolidValidationStrategy.h"

/**
Rejects collapsed laminae: two-face shells whose area is below
{@link PolyhedralBoundedSolidGeometricValidator#collapsedFaceAreaThreshold}.

<p>A lamina (two faces of opposite orientation sharing all their edges,
zero volume) is a valid result: it is topologically sound and easy to
display, and the kernel returns one, for instance, for the intersection of
two blocks that share part of a face. A <em>collapsed</em> lamina is not: its
surface is below the numeric resolution of the kernel, so it is noise from
a degenerate contact, not a shape. Small faces of shells that enclose
volume are not judged here.</p>
*/
class _GeometricCollapsedLaminaStrategy : public _PolyhedralBoundedSolidValidationStrategy {
public:
    virtual BOOLEAN validate(PolyhedralBoundedSolid* solid,
        const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
        String* msg);
};

#endif
