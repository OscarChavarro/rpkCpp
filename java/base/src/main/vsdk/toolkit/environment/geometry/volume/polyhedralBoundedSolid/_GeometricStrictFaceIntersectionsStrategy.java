//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

package vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid;

/**
Checks the non-self-intersection requirement imposed on boundary models in
[MANT1988].15.2, criterion 3, using the geometric intersection tools of
chapter [MANT1988].13.
*/
public class _GeometricStrictFaceIntersectionsStrategy
    implements _PolyhedralBoundedSolidValidationStrategy
{
    /**
    Validates that distinct faces only meet in the ways allowed by
    [MANT1988].15.2, criterion 3, relying on the geometric tests discussed in
    chapter [MANT1988].13.
    */
    @Override
    public boolean validate(PolyhedralBoundedSolid solid,
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext numericContext,
        StringBuilder msg)
    {
        return PolyhedralBoundedSolidGeometricValidator
            .validateFaceIntersectionsStrict(solid, numericContext, msg);
    }
}
