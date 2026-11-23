import type { Vector3Df } from "../linealAlgebra/Vector3Df.js";

export interface KDTreeAdapter<T> {
    getKDPosition(value: T): Vector3Df;
    copy(value: T): T;
}
