//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

package vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid;

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
public class _GeometricCollapsedLaminaStrategy
    implements _PolyhedralBoundedSolidValidationStrategy
{
    @Override
    public boolean validate(PolyhedralBoundedSolid solid,
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext numericContext,
        StringBuilder msg)
    {
        return PolyhedralBoundedSolidGeometricValidator
            .validateNoCollapsedLaminae(solid, numericContext, msg);
    }
}
