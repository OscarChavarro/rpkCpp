//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

import type { StringBuilder } from "../../../../../../java/lang/StringBuilder.js";
import type { PolyhedralBoundedSolid } from "./PolyhedralBoundedSolid.js";
import { PolyhedralBoundedSolidGeometricValidator } from "./PolyhedralBoundedSolidGeometricValidator.js";
import type { ToleranceContext } from "./PolyhedralBoundedSolidNumericPolicy.js";
import type { _PolyhedralBoundedSolidValidationStrategy } from "./_PolyhedralBoundedSolidValidationStrategy.js";

/**
Applies the planar-face consistency expected from the half-edge face model in
[MANT1988].10.2.1 and from the face-equation discussion of [MANT1988].13.1.
*/
export class _GeometricPlanarityStrategy implements _PolyhedralBoundedSolidValidationStrategy {
    /**
    Validates that each face can act as the planar polygon required by
    [MANT1988].10.2.1 and can therefore support the face equation machinery of
    [MANT1988].13.1.
    */
    public validate(solid: PolyhedralBoundedSolid, numericContext: ToleranceContext, msg: StringBuilder): boolean {
        return PolyhedralBoundedSolidGeometricValidator.validateAllFacesPlanarityAndPlanes(solid, numericContext, msg);
    }
}
