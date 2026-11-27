import { Double } from "../../../../../java/lang/Double.js";
import { Math as JavaMath } from "../../../../../java/lang/Math.js";
import type { Matrix4x4d } from "../../../common/linealAlgebra/Matrix4x4d.js";
import type { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import { FloatingInterval } from "../../../common/numerics/FloatingInterval.js";
import { GeometryContainment } from "../element/GeometryContainment.js";
import { GeometryDistanceStatus } from "../element/GeometryDistanceStatus.js";
import { QuadraticInteriorBound } from "./QuadraticInteriorBound.js";

const newIntervalMatrix = (): FloatingInterval[][] => [0, 1, 2].map(() => [0, 1, 2].map(() => new FloatingInterval()));

// A certified enclosure for distance to the surface of an affine sphere.
// The lower bound uses a supporting plane or an interior inverse-norm bound;
// the upper bound uses a surface point in a nonzero local direction. Side may
// be Indeterminate. Outputs change only on Valid.
export class AffineEllipsoidDistanceBounds {
    public static query(
        point: Vector3Dd,
        radius: number,
        forward: Matrix4x4d,
        localDirection: Vector3Dd,
        side: GeometryContainment[],
        lower: number[],
        upper: number[],
        refineInterior = true,
    ): GeometryDistanceStatus {
        if (
            !Double.isFinite(radius) ||
            !(radius > 0) ||
            !Double.isFinite(point.x()) ||
            !Double.isFinite(point.y()) ||
            !Double.isFinite(point.z()) ||
            !Double.isFinite(localDirection.x()) ||
            !Double.isFinite(localDirection.y()) ||
            !Double.isFinite(localDirection.z())
        )
            return GeometryDistanceStatus.InvalidInput;
        for (let i = 0; i < 4; ++i)
            for (let j = 0; j < 4; ++j)
                if (!Double.isFinite(forward.get(i, j))) return GeometryDistanceStatus.InvalidInput;
        if (
            forward.get(3, 0) !== 0 ||
            forward.get(3, 1) !== 0 ||
            forward.get(3, 2) !== 0 ||
            forward.get(3, 3) !== 1 ||
            (localDirection.x() === 0 && localDirection.y() === 0 && localDirection.z() === 0)
        )
            return GeometryDistanceStatus.InvalidInput;
        if (!FloatingInterval.isSupportedEnvironment()) return GeometryDistanceStatus.Unsupported;
        for (let i = 0; i < 3; ++i) {
            if (
                (forward.get(i, 0) === 0 && forward.get(i, 1) === 0 && forward.get(i, 2) === 0) ||
                (forward.get(0, i) === 0 && forward.get(1, i) === 0 && forward.get(2, i) === 0)
            )
                return GeometryDistanceStatus.InvalidInput;
        }
        const a = FloatingInterval.exact(forward.get(0, 0));
        const b = FloatingInterval.exact(forward.get(0, 1));
        const c = FloatingInterval.exact(forward.get(0, 2));
        const d0 = FloatingInterval.exact(forward.get(1, 0));
        const e = FloatingInterval.exact(forward.get(1, 1));
        const f = FloatingInterval.exact(forward.get(1, 2));
        const g = FloatingInterval.exact(forward.get(2, 0));
        const h = FloatingInterval.exact(forward.get(2, 1));
        const i = FloatingInterval.exact(forward.get(2, 2));
        const determinant = a
            .multiply(e.multiply(i).subtract(f.multiply(h)))
            .subtract(b.multiply(d0.multiply(i).subtract(f.multiply(g))))
            .add(c.multiply(d0.multiply(h).subtract(e.multiply(g))));
        if (!determinant.isValid() || (determinant.getLower() <= 0 && determinant.getUpper() >= 0))
            return GeometryDistanceStatus.Indeterminate;
        const p = [point.x(), point.y(), point.z()];
        const d = [localDirection.x(), localDirection.y(), localDirection.z()];
        let lengthSquared = FloatingInterval.exact(0);
        for (let k = 0; k < 3; ++k) lengthSquared = lengthSquared.add(FloatingInterval.exact(d[k]!).square());
        const length = lengthSquared.squareRoot();
        if (!length.isValid() || !(length.getLower() > 0)) return GeometryDistanceStatus.Indeterminate;
        const unit = [new FloatingInterval(), new FloatingInterval(), new FloatingInterval()];
        for (let k = 0; k < 3; ++k)
            unit[k] = FloatingInterval.exact(d[k]!).divide(length).multiply(FloatingInterval.exact(radius));
        let distanceSquared = FloatingInterval.exact(0);
        for (let k = 0; k < 3; ++k) {
            let coordinate = FloatingInterval.exact(forward.get(k, 3));
            for (let j = 0; j < 3; ++j)
                coordinate = coordinate.add(FloatingInterval.exact(forward.get(k, j)).multiply(unit[j]!));
            const delta = coordinate.subtract(FloatingInterval.exact(p[k]!));
            distanceSquared = distanceSquared.add(delta.square());
        }
        const distance = distanceSquared.squareRoot();
        if (!distance.isValid()) return GeometryDistanceStatus.Indeterminate;
        // Any finite nonzero vector n defines a supporting plane. Its distance
        // from the query is a lower bound whenever its signed gap is positive.
        const normals = [
            [0, 0, 0],
            [0, 0, 0],
        ];
        const usable = [false, false];
        for (let k = 0; k < 3; ++k) {
            normals[0]![k] = p[k]! - forward.get(k, 3);
            if (!Double.isFinite(normals[0]![k]!)) return GeometryDistanceStatus.Indeterminate;
            if (normals[0]![k] !== 0) usable[0] = true;
        }
        let lowerBound = 0;
        let certifiedSide = GeometryContainment.Indeterminate;
        // Enclose A^-1 from cofactors of the stored forward matrix itself; the
        // independently stored approximate inverse is not treated as exact.
        const inverse = newIntervalMatrix();
        inverse[0]![0] = e.multiply(i).subtract(f.multiply(h)).divide(determinant);
        inverse[0]![1] = c.multiply(h).subtract(b.multiply(i)).divide(determinant);
        inverse[0]![2] = b.multiply(f).subtract(c.multiply(e)).divide(determinant);
        inverse[1]![0] = f.multiply(g).subtract(d0.multiply(i)).divide(determinant);
        inverse[1]![1] = a.multiply(i).subtract(c.multiply(g)).divide(determinant);
        inverse[1]![2] = c.multiply(d0).subtract(a.multiply(f)).divide(determinant);
        inverse[2]![0] = d0.multiply(h).subtract(e.multiply(g)).divide(determinant);
        inverse[2]![1] = b.multiply(g).subtract(a.multiply(h)).divide(determinant);
        inverse[2]![2] = a.multiply(e).subtract(b.multiply(d0)).divide(determinant);
        let inverseCandidateValid = true;
        for (let row = 0; row < 3; ++row)
            for (let col = 0; col < 3; ++col) {
                if (!inverse[row]![col]!.isValid()) {
                    inverseCandidateValid = false;
                    continue;
                }
                // This rounded midpoint is only a choice of plane direction: any
                // finite direction is valid for the interval-verified support test.
                const midpoint = 0.5 * inverse[row]![col]!.getLower() + 0.5 * inverse[row]![col]!.getUpper();
                normals[1]![col] = normals[1]![col]! + midpoint * d[row]!;
            }
        for (let j = 0; j < 3; ++j) {
            if (!Double.isFinite(normals[1]![j]!)) inverseCandidateValid = false;
            if (normals[1]![j] !== 0) usable[1] = true;
        }
        usable[1] = usable[1]! && inverseCandidateValid;
        let localSquared = FloatingInterval.exact(0);
        let inverseSquared = FloatingInterval.exact(0);
        const local = [new FloatingInterval(), new FloatingInterval(), new FloatingInterval()];
        for (let row = 0; row < 3; ++row) {
            let component = FloatingInterval.exact(0);
            for (let col = 0; col < 3; ++col) {
                component = component.add(
                    inverse[row]![col]!.multiply(
                        FloatingInterval.exact(p[col]!).subtract(FloatingInterval.exact(forward.get(col, 3))),
                    ),
                );
                inverseSquared = inverseSquared.add(inverse[row]![col]!.square());
            }
            local[row] = component;
            localSquared = localSquared.add(component.square());
        }
        const localLength = localSquared.squareRoot();
        const inverseNorm = inverseSquared.squareRoot();
        if (localLength.isValid() && inverseNorm.isValid() && inverseNorm.getLower() > 0) {
            if (localLength.getUpper() < radius) {
                certifiedSide = GeometryContainment.Inside;
                const margin = FloatingInterval.exact(radius).subtract(localLength).divide(inverseNorm);
                if (margin.isValid() && margin.getLower() > 0) lowerBound = margin.getLower();
                // For Q=A^-T A^-1 and g=A^-T x, the exact quadratic obeys
                // q(p+d)<=q(p)+2||g||||d||+||Q||_2||d||^2. Enclose g and Q,
                // bound the symmetric spectral norm by its largest absolute row
                // sum, and solve the scalar quadratic with outward rounding.
                let gradientSquared = FloatingInterval.exact(0);
                const gradientComponents = [new FloatingInterval(), new FloatingInterval(), new FloatingInterval()];
                const quadratic = newIntervalMatrix();
                let maxRow = 0;
                let quadraticValid = true;
                for (let row = 0; row < 3; ++row) {
                    let gradient = FloatingInterval.exact(0);
                    let rowSum = FloatingInterval.exact(0);
                    for (let k = 0; k < 3; ++k) gradient = gradient.add(inverse[k]![row]!.multiply(local[k]!));
                    gradientComponents[row] = gradient;
                    gradientSquared = gradientSquared.add(gradient.square());
                    for (let col = 0; col < 3; ++col) {
                        let element = FloatingInterval.exact(0);
                        for (let k = 0; k < 3; ++k)
                            element = element.add(inverse[k]![row]!.multiply(inverse[k]![col]!));
                        quadratic[row]![col] = element;
                        if (!element.isValid()) {
                            quadraticValid = false;
                            break;
                        }
                        const maxAbs = JavaMath.max(JavaMath.abs(element.getLower()), JavaMath.abs(element.getUpper()));
                        rowSum = rowSum.add(FloatingInterval.exact(maxAbs));
                    }
                    if (!rowSum.isValid()) quadraticValid = false;
                    else if (rowSum.getUpper() > maxRow) maxRow = rowSum.getUpper();
                }
                const gradientLength = gradientSquared.squareRoot();
                const gapSquared = FloatingInterval.exact(radius).square().subtract(localSquared);
                if (quadraticValid && gradientLength.isValid() && gapSquared.isValid() && gapSquared.getLower() > 0) {
                    const gap = FloatingInterval.exact(gapSquared.getLower());
                    const grad = FloatingInterval.exact(gradientLength.getUpper());
                    const curvature = FloatingInterval.exact(maxRow);
                    const denominator = grad.square().add(curvature.multiply(gap)).squareRoot().add(grad);
                    const root = gap.divide(denominator);
                    if (root.isValid() && root.getLower() > lowerBound) lowerBound = root.getLower();
                    if (refineInterior) {
                        const dual = QuadraticInteriorBound.improve(
                            radius,
                            localSquared,
                            gradientComponents,
                            quadratic,
                            maxRow,
                        );
                        if (dual > lowerBound) lowerBound = dual;
                    }
                }
            } else if (localLength.getLower() > radius) certifiedSide = GeometryContainment.Outside;
        }
        for (let candidateIndex = 0; candidateIndex < 2; ++candidateIndex) {
            if (!usable[candidateIndex]) continue;
            const n = normals[candidateIndex]!;
            let projection = FloatingInterval.exact(0);
            let normalSquared = FloatingInterval.exact(0);
            let supportSquared = FloatingInterval.exact(0);
            for (let k = 0; k < 3; ++k) {
                const ni = FloatingInterval.exact(n[k]!);
                normalSquared = normalSquared.add(ni.square());
                projection = projection.add(
                    ni.multiply(FloatingInterval.exact(p[k]!).subtract(FloatingInterval.exact(forward.get(k, 3)))),
                );
                let component = FloatingInterval.exact(0);
                for (let j = 0; j < 3; ++j)
                    component = component.add(
                        FloatingInterval.exact(n[j]!).multiply(FloatingInterval.exact(forward.get(j, k))),
                    );
                supportSquared = supportSquared.add(component.square());
            }
            const support = supportSquared.squareRoot().multiply(FloatingInterval.exact(radius));
            const gap = projection.subtract(support);
            const normalLength = normalSquared.squareRoot();
            if (!gap.isValid() || !normalLength.isValid() || !(normalLength.getLower() > 0))
                return GeometryDistanceStatus.Indeterminate;
            if (gap.getLower() > 0) {
                const gapDistance = gap.divide(normalLength);
                if (!gapDistance.isValid()) return GeometryDistanceStatus.Indeterminate;
                if (certifiedSide === GeometryContainment.Inside) return GeometryDistanceStatus.Indeterminate;
                certifiedSide = GeometryContainment.Outside;
                if (gapDistance.getLower() > lowerBound) lowerBound = gapDistance.getLower();
            }
        }
        const upperBound = distance.getUpper();
        if (!Double.isFinite(upperBound) || lowerBound > upperBound) return GeometryDistanceStatus.Indeterminate;
        side[0] = certifiedSide;
        lower[0] = lowerBound;
        upper[0] = upperBound;
        return GeometryDistanceStatus.Valid;
    }
}
