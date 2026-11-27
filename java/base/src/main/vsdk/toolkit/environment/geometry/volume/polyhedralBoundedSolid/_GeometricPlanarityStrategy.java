//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

package vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid;

/**
Applies the planar-face consistency expected from the half-edge face model in
[MANT1988].10.2.1 and from the face-equation discussion of [MANT1988].13.1.
*/
public class _GeometricPlanarityStrategy
    implements _PolyhedralBoundedSolidValidationStrategy
{
    /**
    Validates that each face can act as the planar polygon required by
    [MANT1988].10.2.1 and can therefore support the face equation machinery of
    [MANT1988].13.1.
    */
    @Override
    public boolean validate(PolyhedralBoundedSolid solid,
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext numericContext,
        StringBuilder msg)
    {
        return PolyhedralBoundedSolidGeometricValidator
            .validateAllFacesPlanarityAndPlanes(solid, numericContext, msg);
    }
}
