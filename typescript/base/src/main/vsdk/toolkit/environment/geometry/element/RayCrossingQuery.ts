import { Double } from "../../../../../java/lang/Double.js";

// Interval in the original affine ray parameterization, including its endpoints.
export class RayCrossingQuery {
    public constructor(
        public minT: number,
        public maxT: number,
        public minimumChordSquared = 0,
        // Compatibility cutoff in normalized-direction distance units.
        public minimumExteriorApproach = Double.NEGATIVE_INFINITY,
    ) {}
}
