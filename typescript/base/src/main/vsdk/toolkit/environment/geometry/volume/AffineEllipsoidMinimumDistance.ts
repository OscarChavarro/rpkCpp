import { Double } from "../../../../../java/lang/Double.js";
import type { Matrix4x4d } from "../../../common/linealAlgebra/Matrix4x4d.js";
import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import { FloatingInterval } from "../../../common/numerics/FloatingInterval.js";
import { GeometryContainment } from "../element/GeometryContainment.js";
import { GeometryDistanceStatus } from "../element/GeometryDistanceStatus.js";
import { AffineEllipsoidDistance } from "./AffineEllipsoidDistance.js";
import { AffineEllipsoidDistanceBounds } from "./AffineEllipsoidDistanceBounds.js";
import { AffineEllipsoidDualBound } from "./AffineEllipsoidDualBound.js";

// Certified enclosure of the minimum world-space surface distance. A valid
// result is at most maxWidth wide; otherwise the status is Indeterminate and
// all outputs remain unchanged. The inverse is only a numerical seed hint.
export class AffineEllipsoidMinimumDistance {
    public static query(
        point: Vector3Dd,
        radius: number,
        forward: Matrix4x4d,
        inverseHint: Matrix4x4d,
        maxWidth: number,
        side: GeometryContainment[],
        lower: number[],
        upper: number[],
    ): GeometryDistanceStatus {
        if (!Double.isFinite(maxWidth) || !(maxWidth > 0)) return GeometryDistanceStatus.InvalidInput;
        for (let i = 0; i < 4; ++i)
            for (let j = 0; j < 4; ++j)
                if (!Double.isFinite(inverseHint.get(i, j))) return GeometryDistanceStatus.InvalidInput;
        const estimate = [0],
            error = [0];
        const witness = [new Vector3Dd()];
        const candidate = AffineEllipsoidDistance.queryWithWitness(point, radius, forward, estimate, error, witness);
        if (candidate === GeometryDistanceStatus.InvalidInput) return candidate;
        let seed =
            candidate === GeometryDistanceStatus.Valid
                ? inverseHint.multiply(witness[0]!)
                : inverseHint.multiply(point);
        if (!Double.isFinite(seed.x()) || !Double.isFinite(seed.y()) || !Double.isFinite(seed.z()))
            return GeometryDistanceStatus.Indeterminate;
        if (seed.x() === 0 && seed.y() === 0 && seed.z() === 0) seed = new Vector3Dd(1, 0, 0);
        const candidateSide = [GeometryContainment.Indeterminate];
        const lo = [0],
            hi = [0];
        const status = AffineEllipsoidDistanceBounds.query(point, radius, forward, seed, candidateSide, lo, hi);
        if (status !== GeometryDistanceStatus.Valid) return status;
        const dual = AffineEllipsoidDualBound.lowerDistance(point, radius, forward, seed);
        if (dual > lo[0]!) lo[0] = dual;
        if (lo[0]! > hi[0]!) return GeometryDistanceStatus.Indeterminate;
        const width = FloatingInterval.exact(hi[0]!).subtract(FloatingInterval.exact(lo[0]!));
        if (!width.isValid() || width.getUpper() > maxWidth) return GeometryDistanceStatus.Indeterminate;
        side[0] = candidateSide[0]!;
        lower[0] = lo[0]!;
        upper[0] = hi[0]!;
        return GeometryDistanceStatus.Valid;
    }
}
