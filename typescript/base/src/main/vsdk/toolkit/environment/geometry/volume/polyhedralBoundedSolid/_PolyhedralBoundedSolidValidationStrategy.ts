//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

import type { StringBuilder } from "../../../../../../java/lang/StringBuilder.js";
import type { PolyhedralBoundedSolid } from "./PolyhedralBoundedSolid.js";
import type { ToleranceContext } from "./PolyhedralBoundedSolidNumericPolicy.js";

/**
Strategy abstraction for checks that preserve the half-edge invariants of
[MANT1988].10 and the geometric validity conditions later used by chapters
[MANT1988].13 and [MANT1988].15.
*/
export interface _PolyhedralBoundedSolidValidationStrategy {
    /**
    Executes one validation criterion derived from the boundary-representation
    invariants discussed in chapters [MANT1988].10, [MANT1988].13, and
    [MANT1988].15.
    */
    validate(solid: PolyhedralBoundedSolid, numericContext: ToleranceContext, msg: StringBuilder): boolean;
}
