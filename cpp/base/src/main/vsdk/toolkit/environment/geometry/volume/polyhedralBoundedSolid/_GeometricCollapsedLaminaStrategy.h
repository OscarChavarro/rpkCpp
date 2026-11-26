//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#ifndef __GEOMETRIC_COLLAPSED_LAMINA_STRATEGY__
#define __GEOMETRIC_COLLAPSED_LAMINA_STRATEGY__

#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_PolyhedralBoundedSolidValidationStrategy.h"

/**
Rejects collapsed laminae: two-face shells whose area is below
`PolyhedralBoundedSolidGeometricValidator::collapsedFaceAreaThreshold`.

A lamina (two faces of opposite orientation sharing all their edges,
zero volume) is a valid result: it is topologically sound and easy to
display, and the kernel returns one, for instance, for the intersection of
two blocks that share part of a face. A collapsed lamina is not: its
surface is below the numeric resolution of the kernel, so it is noise from
a degenerate contact, not a shape. Small faces of shells that enclose
volume are not judged here.
*/
class _GeometricCollapsedLaminaStrategy : public _PolyhedralBoundedSolidValidationStrategy {
public:
    virtual bool validate(PolyhedralBoundedSolid* solid,
        const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
        java::String* msg) override;
};

#endif
