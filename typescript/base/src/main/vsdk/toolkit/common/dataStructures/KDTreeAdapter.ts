import type { Vector3Dd } from "../linealAlgebra/Vector3Dd.js";

export interface KDTreeAdapter<T> {
    getKDPosition(value: T): Vector3Dd;
    copy(value: T): T;
}
