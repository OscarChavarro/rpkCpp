import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import { Volume } from "./Volume.js";
/** A volume with an interior and a center-of-mass query. */
export abstract class Solid extends Volume {
    public doCenterOfMass(): Vector3Dd {
        return new Vector3Dd();
    }
}
