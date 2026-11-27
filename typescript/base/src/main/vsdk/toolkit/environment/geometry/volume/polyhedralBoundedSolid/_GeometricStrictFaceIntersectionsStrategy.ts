//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

import type { StringBuilder } from "../../../../../../java/lang/StringBuilder.js";
import type { PolyhedralBoundedSolid } from "./PolyhedralBoundedSolid.js";
import { PolyhedralBoundedSolidGeometricValidator } from "./PolyhedralBoundedSolidGeometricValidator.js";
import type { ToleranceContext } from "./PolyhedralBoundedSolidNumericPolicy.js";
import type { _PolyhedralBoundedSolidValidationStrategy } from "./_PolyhedralBoundedSolidValidationStrategy.js";

/**
Checks the non-self-intersection requirement imposed on boundary models in
[MANT1988].15.2, criterion 3, using the geometric intersection tools of
chapter [MANT1988].13.
*/
export class _GeometricStrictFaceIntersectionsStrategy implements _PolyhedralBoundedSolidValidationStrategy {
    /**
    Validates that distinct faces only meet in the ways allowed by
    [MANT1988].15.2, criterion 3, relying on the geometric tests discussed in
    chapter [MANT1988].13.
    */
    public validate(solid: PolyhedralBoundedSolid, numericContext: ToleranceContext, msg: StringBuilder): boolean {
        return PolyhedralBoundedSolidGeometricValidator.validateFaceIntersectionsStrict(solid, numericContext, msg);
    }
}
