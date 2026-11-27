//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

package vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid;

/**
Checks loop geometry against the planar-loop expectations of the half-edge
representation from [MANT1988].10.2.1 and the planar polygon predicates of
chapter [MANT1988].13.
*/
public class _GeometricStrictLoopsStrategy
    implements _PolyhedralBoundedSolidValidationStrategy
{
    /**
    Validates that loop boundaries behave as planar polygonal contours, in the
    sense required by [MANT1988].10.2.1 for faces and by chapter [MANT1988].13
    for geometric point-in-polygon style tests.
    */
    @Override
    public boolean validate(PolyhedralBoundedSolid solid,
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext numericContext,
        StringBuilder msg)
    {
        return PolyhedralBoundedSolidGeometricValidator
            .validateLoopsStrict(solid, numericContext, msg);
    }
}
