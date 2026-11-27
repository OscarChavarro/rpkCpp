import type { Vector3Dd } from "../../common/linealAlgebra/Vector3Dd.js";
export interface Material {
    copy(): Material;
    translate(v: Vector3Dd): Material;
    rotate(v: Vector3Dd): Material;
    scale(v: Vector3Dd): Material;
}
