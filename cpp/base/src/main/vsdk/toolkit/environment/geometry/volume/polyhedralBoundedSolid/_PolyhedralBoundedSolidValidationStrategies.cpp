#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidGeometricValidator.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_GeometricCollapsedLaminaStrategy.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_GeometricFaceOrientationStrategy.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_GeometricPlanarityStrategy.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_GeometricStrictFaceIntersectionsStrategy.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_GeometricStrictLoopsStrategy.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_PolyhedralBoundedSolidTopologicalValidator.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/_TopologicalIntegrityStrategy.h"

/**
Validates the basic incidence and cycle properties assumed by the half-edge
representation in [MANT1988].10.2.1 and [MANT1988].10.2.2.
*/
bool _TopologicalIntegrityStrategy::validate(PolyhedralBoundedSolid* solid,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
    java::String* msg)
{
    (void)numericContext;
    // Validation only reads. A dangling pointer left by an operator is
    // reported; repairing it is PolyhedralBoundedSolidRepair's job, on
    // request (plan-stage11 Phase C).
    java::String localMsg;
    bool backPointersOk = _PolyhedralBoundedSolidTopologicalValidator
        ::validateBackPointers(solid, msg != 0 ? msg : &localMsg);
    if ( !_PolyhedralBoundedSolidTopologicalValidator
        ::validateTopologicalIntegrity(solid) ) {
        if ( msg != 0 ) {
            *msg += "  - Topological integrity test failed.\n";
        }
        return false;
    }
    return backPointersOk;
}

/**
Validates that each face can act as the planar polygon required by
[MANT1988].10.2.1 and can therefore support the face equation machinery of
[MANT1988].13.1.
*/
bool _GeometricPlanarityStrategy::validate(PolyhedralBoundedSolid* solid,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
    java::String* msg)
{
    return PolyhedralBoundedSolidGeometricValidator
        ::validateAllFacesPlanarityAndPlanes(solid, numericContext, msg);
}

/**
Delegates to the neighbour-based heuristic in
`PolyhedralBoundedSolidGeometricValidator::validateConsistentFaceOrientations`.
*/
bool _GeometricFaceOrientationStrategy::validate(PolyhedralBoundedSolid* solid,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
    java::String* msg)
{
    return PolyhedralBoundedSolidGeometricValidator
        ::validateConsistentFaceOrientations(solid, numericContext, msg);
}

/**
Validates that loop boundaries behave as planar polygonal contours, in the
sense required by [MANT1988].10.2.1 for faces and by chapter [MANT1988].13
for geometric point-in-polygon style tests.
*/
bool _GeometricStrictLoopsStrategy::validate(PolyhedralBoundedSolid* solid,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
    java::String* msg)
{
    return PolyhedralBoundedSolidGeometricValidator
        ::validateLoopsStrict(solid, numericContext, msg);
}

/**
Rejects collapsed laminae: two-face shells whose area is below
`PolyhedralBoundedSolidGeometricValidator::collapsedFaceAreaThreshold`.
*/
bool _GeometricCollapsedLaminaStrategy::validate(PolyhedralBoundedSolid* solid,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
    java::String* msg)
{
    return PolyhedralBoundedSolidGeometricValidator
        ::validateNoCollapsedLaminae(solid, numericContext, msg);
}

/**
Validates that distinct faces only meet in the ways allowed by
[MANT1988].15.2, criterion 3, relying on the geometric tests discussed in
chapter [MANT1988].13.
*/
bool _GeometricStrictFaceIntersectionsStrategy::validate(PolyhedralBoundedSolid* solid,
    const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext,
    java::String* msg)
{
    return PolyhedralBoundedSolidGeometricValidator
        ::validateFaceIntersectionsStrict(solid, numericContext, msg);
}
