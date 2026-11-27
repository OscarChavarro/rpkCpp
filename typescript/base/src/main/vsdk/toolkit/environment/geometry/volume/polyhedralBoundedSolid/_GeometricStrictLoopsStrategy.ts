//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

import type { StringBuilder } from "../../../../../../java/lang/StringBuilder.js";
import type { PolyhedralBoundedSolid } from "./PolyhedralBoundedSolid.js";
import { PolyhedralBoundedSolidGeometricValidator } from "./PolyhedralBoundedSolidGeometricValidator.js";
import type { ToleranceContext } from "./PolyhedralBoundedSolidNumericPolicy.js";
import type { _PolyhedralBoundedSolidValidationStrategy } from "./_PolyhedralBoundedSolidValidationStrategy.js";

/**
Checks loop geometry against the planar-loop expectations of the half-edge
representation from [MANT1988].10.2.1 and the planar polygon predicates of
chapter [MANT1988].13.
*/
export class _GeometricStrictLoopsStrategy implements _PolyhedralBoundedSolidValidationStrategy {
    /**
    Validates that loop boundaries behave as planar polygonal contours, in the
    sense required by [MANT1988].10.2.1 for faces and by chapter [MANT1988].13
    for geometric point-in-polygon style tests.
    */
    public validate(solid: PolyhedralBoundedSolid, numericContext: ToleranceContext, msg: StringBuilder): boolean {
        return PolyhedralBoundedSolidGeometricValidator.validateLoopsStrict(solid, numericContext, msg);
    }
}
