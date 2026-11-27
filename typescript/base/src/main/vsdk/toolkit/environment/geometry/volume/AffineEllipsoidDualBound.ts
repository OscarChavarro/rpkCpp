import { Double } from "../../../../../java/lang/Double.js";
import { Math as JavaMath } from "../../../../../java/lang/Math.js";
import type { Matrix4x4d } from "../../../common/linealAlgebra/Matrix4x4d.js";
import type { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import { FloatingInterval } from "../../../common/numerics/FloatingInterval.js";

const newIntervalMatrix = (): FloatingInterval[][] => [0, 1, 2].map(() => [0, 1, 2].map(() => new FloatingInterval()));

// Verified lower bound from a Lagrange dual certificate. The supplied local
// direction is only a numerical candidate; failure to certify returns zero.
export class AffineEllipsoidDualBound {
    public static lowerDistance(
        point: Vector3Dd,
        radius: number,
        forward: Matrix4x4d,
        localDirection: Vector3Dd,
    ): number {
        if (!FloatingInterval.isSupportedEnvironment() || !Double.isFinite(radius) || !(radius > 0)) return 0;
        const seedLength = localDirection.length();
        if (!(seedLength > 0) || !Double.isFinite(seedLength)) return 0;
        const unit = localDirection.multiply(1.0 / seedLength);
        const z = [unit.x(), unit.y(), unit.z()];
        const p = [point.x(), point.y(), point.z()];
        const b = newIntervalMatrix(),
            h = newIntervalMatrix();
        const difference = [new FloatingInterval(), new FloatingInterval(), new FloatingInterval()],
            g = [new FloatingInterval(), new FloatingInterval(), new FloatingInterval()];
        let distanceSquared = FloatingInterval.exact(0);
        let scale = 0;
        for (let row = 0; row < 3; ++row) {
            if (!Double.isFinite(z[row]!) || !Double.isFinite(p[row]!)) return 0;
            difference[row] = FloatingInterval.exact(p[row]!).subtract(FloatingInterval.exact(forward.get(row, 3)));
            distanceSquared = distanceSquared.add(difference[row]!.square());
            for (let col = 0; col < 3; ++col)
                b[row]![col] = FloatingInterval.exact(forward.get(row, col)).multiply(FloatingInterval.exact(radius));
        }
        for (let i = 0; i < 3; ++i) {
            g[i] = FloatingInterval.exact(0);
            for (let k = 0; k < 3; ++k) g[i] = g[i]!.add(b[k]![i]!.multiply(difference[k]!));
            if (!g[i]!.isValid()) return 0;
            for (let j = 0; j < 3; ++j) {
                h[i]![j] = FloatingInterval.exact(0);
                for (let k = 0; k < 3; ++k) h[i]![j] = h[i]![j]!.add(b[k]![i]!.multiply(b[k]![j]!));
                if (!h[i]![j]!.isValid()) return 0;
                const magnitude = JavaMath.max(JavaMath.abs(h[i]![j]!.getLower()), JavaMath.abs(h[i]![j]!.getUpper()));
                if (magnitude > scale) scale = magnitude;
            }
        }
        if (!distanceSquared.isValid() || !(scale > 0) || !Double.isFinite(scale)) return 0;
        let estimate = 0;
        for (let i = 0; i < 3; ++i) {
            const gi = 0.5 * g[i]!.getLower() + 0.5 * g[i]!.getUpper();
            estimate += z[i]! * gi;
            for (let j = 0; j < 3; ++j)
                estimate -= z[i]! * z[j]! * (0.5 * h[i]![j]!.getLower() + 0.5 * h[i]![j]!.getUpper());
        }
        if (!Double.isFinite(estimate)) return 0;
        let shift = 1e-13 * (1 + scale + JavaMath.abs(estimate));
        let best = 0;
        for (let attempt = 0; attempt < 20; ++attempt, shift *= 8) {
            const lambda = estimate + shift;
            if (!Double.isFinite(lambda) || !Double.isFinite(shift)) break;
            const matrix = newIntervalMatrix();
            let normUpper = 0;
            let ok = true;
            for (let i = 0; i < 3; ++i) {
                let rowSum = FloatingInterval.exact(0);
                for (let j = 0; j < 3; ++j) {
                    matrix[i]![j] = h[i]![j]!.add(FloatingInterval.exact(i === j ? lambda : 0));
                    if (!matrix[i]![j]!.isValid()) {
                        ok = false;
                        break;
                    }
                    const maxAbs = JavaMath.max(
                        JavaMath.abs(matrix[i]![j]!.getLower()),
                        JavaMath.abs(matrix[i]![j]!.getUpper()),
                    );
                    rowSum = rowSum.add(FloatingInterval.exact(maxAbs));
                }
                if (!rowSum.isValid()) ok = false;
                else if (rowSum.getUpper() > normUpper) normUpper = rowSum.getUpper();
            }
            if (!ok || !(normUpper > 0)) continue;
            const m = matrix;
            const minor2 = m[0]![0]!.multiply(m[1]![1]!).subtract(m[0]![1]!.multiply(m[1]![0]!));
            const det = m[0]![0]!
                .multiply(m[1]![1]!.multiply(m[2]![2]!).subtract(m[1]![2]!.multiply(m[2]![1]!)))
                .subtract(m[0]![1]!.multiply(m[1]![0]!.multiply(m[2]![2]!).subtract(m[1]![2]!.multiply(m[2]![0]!))))
                .add(m[0]![2]!.multiply(m[1]![0]!.multiply(m[2]![1]!).subtract(m[1]![1]!.multiply(m[2]![0]!))));
            if (
                !minor2.isValid() ||
                !det.isValid() ||
                !(m[0]![0]!.getLower() > 0) ||
                !(minor2.getLower() > 0) ||
                !(det.getLower() > 0)
            )
                continue;
            // Sylvester proves positive definiteness. det/||P||^2 is a lower
            // bound for its smallest eigenvalue because both other eigenvalues
            // are at most the symmetric maximum absolute row sum.
            const eigenLower = det.divide(FloatingInterval.exact(normUpper).square());
            if (!eigenLower.isValid() || !(eigenLower.getLower() > 0)) continue;
            let dot = FloatingInterval.exact(0),
                quadratic = FloatingInterval.exact(0);
            let residualSquared = FloatingInterval.exact(0);
            for (let i = 0; i < 3; ++i) {
                const zi = FloatingInterval.exact(z[i]!);
                dot = dot.add(g[i]!.multiply(zi));
                let residual = g[i]!;
                for (let j = 0; j < 3; ++j) {
                    const pz = m[i]![j]!.multiply(FloatingInterval.exact(z[j]!));
                    residual = residual.subtract(pz);
                    quadratic = quadratic.add(zi.multiply(pz));
                }
                residualSquared = residualSquared.add(residual.square());
            }
            const correction = dot
                .multiply(FloatingInterval.exact(2))
                .subtract(quadratic)
                .add(residualSquared.divide(FloatingInterval.exact(eigenLower.getLower())));
            const lowerSquared = distanceSquared.subtract(FloatingInterval.exact(lambda)).subtract(correction);
            if (!lowerSquared.isValid() || !(lowerSquared.getLower() > 0)) continue;
            const result = FloatingInterval.exact(lowerSquared.getLower()).squareRoot();
            if (result.isValid() && result.getLower() > best) best = result.getLower();
        }
        return best;
    }
}
