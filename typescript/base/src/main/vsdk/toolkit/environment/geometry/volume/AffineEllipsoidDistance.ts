import { Double } from "../../../../../java/lang/Double.js";
import { Math as JavaMath } from "../../../../../java/lang/Math.js";
import { Matrix4x4d } from "../../../common/linealAlgebra/Matrix4x4d.js";
import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import { GeometryDistanceStatus } from "../element/GeometryDistanceStatus.js";
import { OrthogonalEllipsoidDistance } from "./OrthogonalEllipsoidDistance.js";

// Estimates Euclidean distance to an affine sphere, including shear. Returns
// an explicit status and an estimated (not formally proven) numerical guard.
// Outputs are assigned only on Valid. Do not treat the estimate as a certified
// interval or use it to force a CSG classification near the tolerance band.
export class AffineEllipsoidDistance {
    public static query(
        point: Vector3Dd,
        radius: number,
        forward: Matrix4x4d,
        distance: number[],
        estimatedError: number[],
    ): GeometryDistanceStatus {
        const witness = [new Vector3Dd()];
        return AffineEllipsoidDistance.queryWithWitness(point, radius, forward, distance, estimatedError, witness);
    }

    public static queryWithWitness(
        point: Vector3Dd,
        radius: number,
        forward: Matrix4x4d,
        distance: number[],
        estimatedError: number[],
        witness: Vector3Dd[],
    ): GeometryDistanceStatus {
        const candidate = [0],
            guard = [0];
        const candidateWitness = [new Vector3Dd()];
        const direct = OrthogonalEllipsoidDistance.queryWithWitness(
            point,
            radius,
            forward,
            candidate,
            guard,
            candidateWitness,
        );
        if (direct === GeometryDistanceStatus.Valid) {
            distance[0] = candidate[0]!;
            estimatedError[0] = guard[0]!;
            witness[0] = candidateWitness[0]!;
            return direct;
        }
        if (direct !== GeometryDistanceStatus.Unsupported) return direct;
        // A transformed sphere has principal axes given by the eigenvectors of
        // A A^T. Scale A before forming the product to reduce overflow risk.
        let scale = 0;
        for (let i = 0; i < 3; ++i)
            for (let j = 0; j < 3; ++j)
                if (JavaMath.abs(forward.get(i, j)) > scale) scale = JavaMath.abs(forward.get(i, j));
        if (!(scale > 0) || !Double.isFinite(scale) || !Double.isFinite(scale * radius))
            return GeometryDistanceStatus.Indeterminate;
        const matrix = [
            [0, 0, 0],
            [0, 0, 0],
            [0, 0, 0],
        ];
        const vectors = [
            [1, 0, 0],
            [0, 1, 0],
            [0, 0, 1],
        ];
        for (let i = 0; i < 3; ++i)
            for (let j = 0; j < 3; ++j)
                for (let k = 0; k < 3; ++k)
                    matrix[i]![j] = matrix[i]![j]! + (forward.get(i, k) / scale) * (forward.get(j, k) / scale);
        let converged = false;
        for (let iteration = 0; iteration < 80; ++iteration) {
            let p = 0,
                q = 1;
            if (JavaMath.abs(matrix[0]![2]!) > JavaMath.abs(matrix[p]![q]!)) {
                p = 0;
                q = 2;
            }
            if (JavaMath.abs(matrix[1]![2]!) > JavaMath.abs(matrix[p]![q]!)) {
                p = 1;
                q = 2;
            }
            const off = JavaMath.abs(matrix[p]![q]!);
            const diagonal = JavaMath.abs(matrix[0]![0]!) + JavaMath.abs(matrix[1]![1]!) + JavaMath.abs(matrix[2]![2]!);
            if (off <= 1e-14 * diagonal) {
                converged = true;
                break;
            }
            const tau = (matrix[q]![q]! - matrix[p]![p]!) / (2 * matrix[p]![q]!);
            if (!Double.isFinite(tau)) return GeometryDistanceStatus.Indeterminate;
            const sign = tau < 0 ? -1.0 : 1.0;
            const tangent = sign / (JavaMath.abs(tau) + JavaMath.sqrt(1 + tau * tau));
            const cosine = 1 / JavaMath.sqrt(1 + tangent * tangent),
                sine = tangent * cosine;
            if (!Double.isFinite(cosine) || !Double.isFinite(sine)) return GeometryDistanceStatus.Indeterminate;
            const app = matrix[p]![p]!,
                aqq = matrix[q]![q]!,
                apq = matrix[p]![q]!;
            matrix[p]![p] = app - tangent * apq;
            matrix[q]![q] = aqq + tangent * apq;
            matrix[p]![q] = matrix[q]![p] = 0;
            for (let k = 0; k < 3; ++k) {
                if (k !== p && k !== q) {
                    const kp = matrix[k]![p]!,
                        kq = matrix[k]![q]!;
                    matrix[k]![p] = matrix[p]![k] = cosine * kp - sine * kq;
                    matrix[k]![q] = matrix[q]![k] = sine * kp + cosine * kq;
                }
                const vp = vectors[k]![p]!,
                    vq = vectors[k]![q]!;
                vectors[k]![p] = cosine * vp - sine * vq;
                vectors[k]![q] = sine * vp + cosine * vq;
            }
        }
        if (!converged) return GeometryDistanceStatus.Indeterminate;
        // Reject a decomposition whose reconstructed covariance has drifted
        // noticeably from the original. This residual check is not a formal
        // floating-point error bound.
        for (let i = 0; i < 3; ++i)
            for (let j = 0; j < 3; ++j) {
                let original = 0,
                    reconstructed = 0;
                for (let k = 0; k < 3; ++k) {
                    original += (forward.get(i, k) / scale) * (forward.get(j, k) / scale);
                    reconstructed += vectors[i]![k]! * matrix[k]![k]! * vectors[j]![k]!;
                }
                if (
                    !Double.isFinite(original) ||
                    !Double.isFinite(reconstructed) ||
                    JavaMath.abs(original - reconstructed) > 1e-11 * (1 + JavaMath.abs(original))
                )
                    return GeometryDistanceStatus.Indeterminate;
            }
        let principal = new Matrix4x4d();
        for (let i = 0; i < 3; ++i) {
            if (!(matrix[i]![i]! > 1e-12) || !Double.isFinite(matrix[i]![i]!))
                return GeometryDistanceStatus.Indeterminate;
            const axis = scale * radius * JavaMath.sqrt(matrix[i]![i]!);
            if (!Double.isFinite(axis) || !(axis > 0)) return GeometryDistanceStatus.Indeterminate;
            for (let j = 0; j < 3; ++j) principal = principal.withVal(j, i, vectors[j]![i]! * axis);
            principal = principal.withVal(i, 3, forward.get(i, 3));
        }
        const result = OrthogonalEllipsoidDistance.queryWithWitness(
            point,
            1,
            principal,
            candidate,
            guard,
            candidateWitness,
        );
        if (result === GeometryDistanceStatus.Unsupported) return GeometryDistanceStatus.Indeterminate;
        if (result !== GeometryDistanceStatus.Valid) return result;
        const totalGuard = guard[0]! + 1e-9 * (1 + scale * radius);
        if (!Double.isFinite(totalGuard)) return GeometryDistanceStatus.Indeterminate;
        distance[0] = candidate[0]!;
        estimatedError[0] = totalGuard;
        witness[0] = candidateWitness[0]!;
        return GeometryDistanceStatus.Valid;
    }
}
