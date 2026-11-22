import { Vector3Df } from "vitral/dist/vsdk/toolkit/common/linealAlgebra/Vector3Df";

export class Ray {
    public position: Vector3Df;
    public direction: Vector3Df;

    public constructor(position?: Vector3Df, direction?: Vector3Df) {
        this.position = position ?? new Vector3Df();
        this.direction = direction ?? new Vector3Df();
    }
}
