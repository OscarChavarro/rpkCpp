//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

package vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid;

/**
Applies topological consistency checks for the half-edge data structure of
[MANT1988].10.2.1 and [MANT1988].10.2.2.
*/
public class _TopologicalIntegrityStrategy
    implements _PolyhedralBoundedSolidValidationStrategy
{
    /**
    Validates the basic incidence and cycle properties assumed by the half-edge
    representation in [MANT1988].10.2.1 and [MANT1988].10.2.2.
    */
    @Override
    public boolean validate(PolyhedralBoundedSolid solid,
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext numericContext,
        StringBuilder msg)
    {
        // Validation only reads. A dangling pointer left by an operator is
        // reported; repairing it is PolyhedralBoundedSolidRepair's job, on
        // request (plan-stage11 Phase C).
        boolean backPointersOk = _PolyhedralBoundedSolidTopologicalValidator
            .validateBackPointers(solid, msg);
        if ( !_PolyhedralBoundedSolidTopologicalValidator
            .validateTopologicalIntegrity(solid) ) {
            msg.append("  - Topological integrity test failed.\n");
            return false;
        }
        return backPointersOk;
    }
}
