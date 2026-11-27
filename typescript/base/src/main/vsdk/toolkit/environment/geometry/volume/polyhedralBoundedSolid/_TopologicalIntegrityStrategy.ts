//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

import type { StringBuilder } from "../../../../../../java/lang/StringBuilder.js";
import type { PolyhedralBoundedSolid } from "./PolyhedralBoundedSolid.js";
import type { ToleranceContext } from "./PolyhedralBoundedSolidNumericPolicy.js";
import type { _PolyhedralBoundedSolidValidationStrategy } from "./_PolyhedralBoundedSolidValidationStrategy.js";
import { _PolyhedralBoundedSolidTopologicalValidator } from "./_PolyhedralBoundedSolidTopologicalValidator.js";

/**
Applies topological consistency checks for the half-edge data structure of
[MANT1988].10.2.1 and [MANT1988].10.2.2.
*/
export class _TopologicalIntegrityStrategy implements _PolyhedralBoundedSolidValidationStrategy {
    /**
    Validates the basic incidence and cycle properties assumed by the half-edge
    representation in [MANT1988].10.2.1 and [MANT1988].10.2.2.
    */
    public validate(solid: PolyhedralBoundedSolid, _numericContext: ToleranceContext, msg: StringBuilder): boolean {
        // Validation only reads. A dangling pointer left by an operator is
        // reported; repairing it is PolyhedralBoundedSolidRepair's job, on
        // request (plan-stage11 Phase C).
        const backPointersOk = _PolyhedralBoundedSolidTopologicalValidator.validateBackPointers(solid, msg);
        if (!_PolyhedralBoundedSolidTopologicalValidator.validateTopologicalIntegrity(solid)) {
            msg.append("  - Topological integrity test failed.\n");
            return false;
        }
        return backPointersOk;
    }
}
