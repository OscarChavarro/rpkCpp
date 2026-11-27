import { Double } from "../../../../../java/lang/Double.js";
import { Math as JavaMath } from "../../../../../java/lang/Math.js";
import type { Matrix4x4d } from "../../../common/linealAlgebra/Matrix4x4d.js";
import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import { GeometryDistanceStatus } from "../element/GeometryDistanceStatus.js";

// Euclidean distance to an affine sphere whose three transformed axes are
// orthogonal. The error estimate is a numerical guard, not a proven bound.
export class OrthogonalEllipsoidDistance {
    public static query(
        point: Vector3Dd,
        radius: number,
        forward: Matrix4x4d,
        distance: number[],
        estimatedError: number[],
    ): GeometryDistanceStatus {
        const witness = [new Vector3Dd()];
        return OrthogonalEllipsoidDistance.queryWithWitness(point, radius, forward, distance, estimatedError, witness);
    }

    public static queryWithWitness(
        point: Vector3Dd,
        radius: number,
        forward: Matrix4x4d,
        distance: number[],
        estimatedError: number[],
        witness: Vector3Dd[],
    ): GeometryDistanceStatus {
        if (
            !Double.isFinite(radius) ||
            !(radius > 0) ||
            !Double.isFinite(point.x()) ||
            !Double.isFinite(point.y()) ||
            !Double.isFinite(point.z())
        )
            return GeometryDistanceStatus.InvalidInput;
        for (let i = 0; i < 4; ++i)
            for (let j = 0; j < 4; ++j)
                if (!Double.isFinite(forward.get(i, j))) return GeometryDistanceStatus.InvalidInput;
        if (forward.get(3, 0) !== 0 || forward.get(3, 1) !== 0 || forward.get(3, 2) !== 0 || forward.get(3, 3) !== 1)
            return GeometryDistanceStatus.InvalidInput;
        const p = [point.x(), point.y(), point.z()];
        const axis = [
            [0, 0, 0],
            [0, 0, 0],
            [0, 0, 0],
        ];
        const length = [0, 0, 0],
            x = [0, 0, 0],
            a = [0, 0, 0];
        let maxAxis = 0,
            minAxis = Double.MAX_VALUE,
            magnitude = 1;
        for (let i = 0; i < 3; ++i) {
            let sum = 0;
            for (let j = 0; j < 3; ++j) {
                axis[i]![j] = forward.get(j, i);
                sum += axis[i]![j]! * axis[i]![j]!;
            }
            length[i] = JavaMath.sqrt(sum);
            if (!Double.isFinite(length[i]!)) return GeometryDistanceStatus.Indeterminate;
            if (!(length[i]! > 0)) {
                // A nonzero column can have a squared norm that underflows; that
                // is not proof of a singular transformation.
                if (axis[i]![0] !== 0 || axis[i]![1] !== 0 || axis[i]![2] !== 0)
                    return GeometryDistanceStatus.Indeterminate;
                return GeometryDistanceStatus.InvalidInput;
            }
            a[i] = radius * length[i]!;
            if (!Double.isFinite(a[i]!) || !(a[i]! > 0)) return GeometryDistanceStatus.Indeterminate;
            if (a[i]! > maxAxis) maxAxis = a[i]!;
            if (a[i]! < minAxis) minAxis = a[i]!;
            for (let j = 0; j < 3; ++j) axis[i]![j] = axis[i]![j]! / length[i]!;
        }
        if (minAxis / maxAxis < 1e-6) return GeometryDistanceStatus.Indeterminate;
        for (let i = 0; i < 3; ++i)
            for (let j = 0; j < i; ++j) {
                let dot = 0;
                for (let k = 0; k < 3; ++k) dot += axis[i]![k]! * axis[j]![k]!;
                if (!Double.isFinite(dot)) return GeometryDistanceStatus.Indeterminate;
                if (JavaMath.abs(dot) > 1e-12) return GeometryDistanceStatus.Unsupported;
            }
        for (let i = 0; i < 3; ++i) {
            x[i] = 0;
            for (let j = 0; j < 3; ++j) {
                const delta = p[j]! - forward.get(j, 3);
                x[i] = x[i]! + (axis[i]![j]! * delta) / maxAxis;
                magnitude += JavaMath.abs(p[j]!) + JavaMath.abs(forward.get(j, 3));
            }
            a[i] = a[i]! / maxAxis;
            if (!Double.isFinite(x[i]!)) return GeometryDistanceStatus.Indeterminate;
        }
        let implicit = 0;
        for (let i = 0; i < 3; ++i) implicit += (x[i]! / a[i]!) * (x[i]! / a[i]!);
        if (!Double.isFinite(implicit)) return GeometryDistanceStatus.Indeterminate;
        const y = [0, 0, 0];
        let computedDistance = 0;
        if (implicit === 1) {
            computedDistance = 0;
            for (let i = 0; i < 3; ++i) y[i] = x[i]!;
        } else {
            const inside = implicit < 1;
            let minimumSquared = 1;
            for (let i = 0; i < 3; ++i) if (a[i]! * a[i]! < minimumSquared) minimumSquared = a[i]! * a[i]!;
            let hard = inside;
            let hardSum = 0;
            let shortest = 0;
            if (inside)
                for (let i = 0; i < 3; ++i) {
                    const sq = a[i]! * a[i]!;
                    if (sq === minimumSquared) {
                        shortest = i;
                        if (x[i] !== 0) hard = false;
                    } else {
                        const ratio = (a[i]! * x[i]!) / (sq - minimumSquared);
                        hardSum += ratio * ratio;
                    }
                }
            if (hard && Double.isFinite(hardSum) && hardSum <= 1) {
                for (let i = 0; i < 3; ++i) {
                    const sq = a[i]! * a[i]!;
                    if (sq !== minimumSquared) y[i] = (sq * x[i]!) / (sq - minimumSquared);
                }
                y[shortest] = a[shortest]! * JavaMath.sqrt(1 - hardSum);
            } else {
                let lo = inside ? -minimumSquared : 0,
                    hi = inside ? 0 : 1;
                if (!inside) {
                    let bracketed = false;
                    for (let iteration = 0; iteration < 128; ++iteration) {
                        let f = 0;
                        for (let i = 0; i < 3; ++i) {
                            const z = (a[i]! * x[i]!) / (a[i]! * a[i]! + hi);
                            f += z * z;
                        }
                        if (Double.isFinite(f) && f <= 1) {
                            bracketed = true;
                            break;
                        }
                        hi *= 2;
                        if (!Double.isFinite(hi)) break;
                    }
                    if (!bracketed) return GeometryDistanceStatus.Indeterminate;
                }
                for (let iteration = 0; iteration < 128; ++iteration) {
                    const mid = lo + (hi - lo) * 0.5;
                    if (mid === lo || mid === hi) break;
                    let f = 0;
                    for (let i = 0; i < 3; ++i) {
                        const z = (a[i]! * x[i]!) / (a[i]! * a[i]! + mid);
                        f += z * z;
                    }
                    if (!Double.isFinite(f)) return GeometryDistanceStatus.Indeterminate;
                    if (f > 1) lo = mid;
                    else hi = mid;
                }
                const lambda = lo + (hi - lo) * 0.5;
                let surfaceCheck = 0;
                for (let i = 0; i < 3; ++i) {
                    const denominator = a[i]! * a[i]! + lambda;
                    if (!(denominator > 0)) return GeometryDistanceStatus.Indeterminate;
                    y[i] = (a[i]! * a[i]! * x[i]!) / denominator;
                    const z = y[i]! / a[i]!;
                    surfaceCheck += z * z;
                }
                if (!Double.isFinite(surfaceCheck) || JavaMath.abs(surfaceCheck - 1) > 1e-8)
                    return GeometryDistanceStatus.Indeterminate;
            }
            let squaredDistance = 0;
            for (let i = 0; i < 3; ++i) {
                const delta = y[i]! - x[i]!;
                squaredDistance += delta * delta;
            }
            computedDistance = maxAxis * JavaMath.sqrt(squaredDistance);
        }
        const computedError = 1e-9 * (magnitude + maxAxis + computedDistance);
        if (!Double.isFinite(computedDistance) || !Double.isFinite(computedError))
            return GeometryDistanceStatus.Indeterminate;
        const coordinates = [forward.get(0, 3), forward.get(1, 3), forward.get(2, 3)];
        for (let i = 0; i < 3; ++i)
            for (let j = 0; j < 3; ++j) coordinates[j] = coordinates[j]! + axis[i]![j]! * y[i]! * maxAxis;
        if (!Double.isFinite(coordinates[0]!) || !Double.isFinite(coordinates[1]!) || !Double.isFinite(coordinates[2]!))
            return GeometryDistanceStatus.Indeterminate;
        distance[0] = computedDistance;
        estimatedError[0] = computedError;
        witness[0] = new Vector3Dd(coordinates[0]!, coordinates[1]!, coordinates[2]!);
        return GeometryDistanceStatus.Valid;
    }
}
