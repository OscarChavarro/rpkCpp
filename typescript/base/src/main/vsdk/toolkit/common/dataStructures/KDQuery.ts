import type { Vector3Dd } from "../linealAlgebra/Vector3Dd.js";

export class KDQuery<T> {
    public found = 0;
    public constructor(
        public readonly point: Vector3Dd,
        public readonly wanted: number,
        public readonly results: T[],
        public readonly distances: number[],
        public maximumDistance: number,
        public readonly squaredRadius: number,
        public readonly excludeFlags: number,
    ) {}
}
