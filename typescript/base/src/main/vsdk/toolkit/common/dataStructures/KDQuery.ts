import type { Vector3Df } from "../linealAlgebra/Vector3Df.js";

export class KDQuery<T> {
    public found = 0;
    public constructor(
        public readonly point: Vector3Df,
        public readonly wanted: number,
        public readonly results: T[],
        public readonly distances: number[],
        public maximumDistance: number,
        public readonly squaredRadius: number,
        public readonly excludeFlags: number,
    ) {}
}
