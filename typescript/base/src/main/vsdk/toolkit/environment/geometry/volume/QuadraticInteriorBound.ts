import { Double } from "../../../../../java/lang/Double.js";
import { FloatingInterval } from "../../../common/numerics/FloatingInterval.js";

// Certified lower distance bound for the interior of an affine sphere.
// Inputs enclose the local squared length, gradient A^-T x, symmetric matrix
// A^-T A^-1, and an upper bound on its spectral norm. Returns zero if no
// positive bound can be verified. Caller checks the interval environment.
export class QuadraticInteriorBound {
    public static improve(
        radius: number,
        localSquared: FloatingInterval,
        gradient: FloatingInterval[],
        quadratic: FloatingInterval[][],
        spectralUpper: number,
    ): number {
        if (!(spectralUpper > 0) || !Double.isFinite(spectralUpper)) return 0;
        const radiusSquared = FloatingInterval.exact(radius).square();
        const q = [
            [0, 0, 0],
            [0, 0, 0],
            [0, 0, 0],
        ];
        const g = [0, 0, 0];
        for (let i = 0; i < 3; ++i) {
            if (!gradient[i]!.isValid()) return 0;
            g[i] = 0.5 * gradient[i]!.getLower() + 0.5 * gradient[i]!.getUpper();
            if (!Double.isFinite(g[i]!)) return 0;
            for (let j = 0; j < 3; ++j) {
                if (!quadratic[i]![j]!.isValid()) return 0;
                q[i]![j] = 0.5 * quadratic[i]![j]!.getLower() + 0.5 * quadratic[i]![j]!.getUpper();
                if (!Double.isFinite(q[i]![j]!)) return 0;
            }
        }
        let best = 0;
        let extra = spectralUpper * 0.005;
        if (!(extra > 0) || !Double.isFinite(extra)) return 0;
        for (let candidate = 0; candidate < 16; ++candidate, extra *= 2) {
            const lambda = spectralUpper + extra;
            if (!Double.isFinite(lambda) || !(lambda > spectralUpper)) break;
            // Fixed-point iteration only proposes z. Correctness does not depend
            // on convergence: the interval residual below accounts for its error.
            const z = [0, 0, 0];
            let finite = true;
            for (let iteration = 0; iteration < 24; ++iteration) {
                const next = [0, 0, 0];
                for (let i = 0; i < 3; ++i) {
                    next[i] = g[i]!;
                    for (let j = 0; j < 3; ++j) next[i] = next[i]! + q[i]![j]! * z[j]!;
                    next[i] = next[i]! / lambda;
                    if (!Double.isFinite(next[i]!)) finite = false;
                }
                if (!finite) break;
                for (let i = 0; i < 3; ++i) z[i] = next[i]!;
            }
            if (!finite) continue;
            const delta = FloatingInterval.exact(lambda).subtract(FloatingInterval.exact(spectralUpper));
            if (!delta.isValid() || !(delta.getLower() > 0)) continue;
            let dot = FloatingInterval.exact(0);
            let quadraticTerm = FloatingInterval.exact(0);
            let residualSquared = FloatingInterval.exact(0);
            for (let i = 0; i < 3; ++i) {
                const zi = FloatingInterval.exact(z[i]!);
                dot = dot.add(gradient[i]!.multiply(zi));
                let residual = gradient[i]!;
                for (let j = 0; j < 3; ++j) {
                    const p = FloatingInterval.exact(i === j ? lambda : 0).subtract(quadratic[i]![j]!);
                    const pz = p.multiply(FloatingInterval.exact(z[j]!));
                    residual = residual.subtract(pz);
                    quadraticTerm = quadraticTerm.add(zi.multiply(pz));
                }
                residualSquared = residualSquared.add(residual.square());
            }
            // P=lambda I-Q is positive definite and lambda_min(P)>=delta.
            // Completing the square at arbitrary z bounds the exact maximum by
            // 2g.z-z.P.z + ||g-Pz||^2/delta.
            const correction = dot
                .multiply(FloatingInterval.exact(2))
                .subtract(quadraticTerm)
                .add(residualSquared.divide(delta));
            const gap = radiusSquared.subtract(localSquared).subtract(correction);
            if (!gap.isValid() || !(gap.getLower() > 0)) continue;
            const root = gap.divide(FloatingInterval.exact(lambda)).squareRoot();
            if (root.isValid() && root.getLower() > best) best = root.getLower();
        }
        return best;
    }
}
