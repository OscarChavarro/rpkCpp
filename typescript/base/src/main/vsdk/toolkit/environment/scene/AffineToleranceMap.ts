import { Double } from "../../../../java/lang/Double.js";
import { Math as JavaMath } from "../../../../java/lang/Math.js";
import type { Matrix4x4d } from "../../common/linealAlgebra/Matrix4x4d.js";
import { Vector3Dd } from "../../common/linealAlgebra/Vector3Dd.js";
import { FloatingInterval } from "../../common/numerics/FloatingInterval.js";
import { AffinePlacementStatus } from "./AffinePlacementStatus.js";

const newIntervalMatrix = (): FloatingInterval[][] => [0, 1, 2].map(() => [0, 1, 2].map(() => new FloatingInterval()));

// Enclose the inverse image of a parent-space tolerance ball in a child-space
// ball and, when requested, provide a child ball enclosed in that inverse image.
// A negative inner radius means no positive inner ball was certified.
// childHalfWidths, when requested, bounds the same inverse image by an
// axis-aligned child box, which is tighter under anisotropic placement.
export class AffineToleranceMap {
    public static map(
        forward: Matrix4x4d,
        point: Vector3Dd,
        tolerance: number,
        childPoint: Vector3Dd[],
        childTolerance: number[],
        childInnerTolerance: number[] | null = null,
        childHalfWidths: Vector3Dd[] | null = null,
    ): AffinePlacementStatus {
        if (
            !Double.isFinite(point.x()) ||
            !Double.isFinite(point.y()) ||
            !Double.isFinite(point.z()) ||
            !Double.isFinite(tolerance) ||
            tolerance < 0
        )
            return AffinePlacementStatus.Invalid;
        if (forward.get(3, 0) !== 0 || forward.get(3, 1) !== 0 || forward.get(3, 2) !== 0 || forward.get(3, 3) !== 1)
            return AffinePlacementStatus.Invalid;
        if (!FloatingInterval.isSupportedEnvironment()) return AffinePlacementStatus.Unsupported;
        const matrix = newIntervalMatrix(),
            cofactor = newIntervalMatrix(),
            inverse = newIntervalMatrix();
        for (let i = 0; i < 3; ++i)
            for (let j = 0; j < 3; ++j) {
                matrix[i]![j] = FloatingInterval.exact(forward.get(i, j));
                if (!matrix[i]![j]!.isValid()) return AffinePlacementStatus.Invalid;
            }
        for (let i = 0; i < 3; ++i)
            for (let j = 0; j < 3; ++j) {
                const a = (i + 1) % 3,
                    b = (i + 2) % 3,
                    c = (j + 1) % 3,
                    d = (j + 2) % 3;
                cofactor[i]![j] = matrix[a]![c]!.multiply(matrix[b]![d]!).subtract(
                    matrix[a]![d]!.multiply(matrix[b]![c]!),
                );
            }
        let determinant = FloatingInterval.exact(0);
        for (let j = 0; j < 3; ++j) determinant = determinant.add(matrix[0]![j]!.multiply(cofactor[0]![j]!));
        if (!determinant.isValid() || (determinant.getLower() <= 0 && determinant.getUpper() >= 0))
            return AffinePlacementStatus.Indeterminate;
        for (let i = 0; i < 3; ++i)
            for (let j = 0; j < 3; ++j) {
                inverse[i]![j] = cofactor[j]![i]!.divide(determinant);
                if (!inverse[i]![j]!.isValid()) return AffinePlacementStatus.Indeterminate;
            }
        const coordinates = [point.x(), point.y(), point.z()];
        const delta = [new FloatingInterval(), new FloatingInterval(), new FloatingInterval()];
        for (let i = 0; i < 3; ++i) {
            delta[i] = FloatingInterval.exact(coordinates[i]!).subtract(FloatingInterval.exact(forward.get(i, 3)));
            if (!delta[i]!.isValid()) return AffinePlacementStatus.Indeterminate;
        }
        const center = [0, 0, 0],
            halfWidth = [0, 0, 0];
        let total = FloatingInterval.exact(0);
        for (let i = 0; i < 3; ++i) {
            let mapped = FloatingInterval.exact(0),
                radius = FloatingInterval.exact(0);
            for (let j = 0; j < 3; ++j) {
                mapped = mapped.add(inverse[i]![j]!.multiply(delta[j]!));
                const magnitude = JavaMath.max(
                    JavaMath.abs(inverse[i]![j]!.getLower()),
                    JavaMath.abs(inverse[i]![j]!.getUpper()),
                );
                radius = radius.add(FloatingInterval.exact(magnitude).multiply(FloatingInterval.exact(tolerance)));
            }
            if (!mapped.isValid() || !radius.isValid()) return AffinePlacementStatus.Indeterminate;
            center[i] = mapped.getLower() * 0.5 + mapped.getUpper() * 0.5;
            if (!Double.isFinite(center[i]!)) return AffinePlacementStatus.Indeterminate;
            const error = mapped.subtract(FloatingInterval.exact(center[i]!));
            if (!error.isValid()) return AffinePlacementStatus.Indeterminate;
            const errorBound = JavaMath.max(JavaMath.abs(error.getLower()), JavaMath.abs(error.getUpper()));
            radius = radius.add(FloatingInterval.exact(errorBound));
            total = total.add(radius.square());
            // Along axis i the image of the unit ball under the inverse extends by
            // the Euclidean norm of row i, bounded here using the row's magnitudes.
            let rowSquared = FloatingInterval.exact(0);
            for (let j = 0; j < 3; ++j) {
                const magnitude = JavaMath.max(
                    JavaMath.abs(inverse[i]![j]!.getLower()),
                    JavaMath.abs(inverse[i]![j]!.getUpper()),
                );
                rowSquared = rowSquared.add(FloatingInterval.exact(magnitude).square());
            }
            const width = rowSquared
                .squareRoot()
                .multiply(FloatingInterval.exact(tolerance))
                .add(FloatingInterval.exact(errorBound));
            if (!width.isValid()) return AffinePlacementStatus.Indeterminate;
            halfWidth[i] = width.getUpper();
        }
        const bound = total.squareRoot();
        if (!bound.isValid()) return AffinePlacementStatus.Indeterminate;
        let inner = -1;
        if (childInnerTolerance && tolerance > 0) {
            // A child ball of radius r maps inside the world ball when
            // ||A center + translation - point|| + ||A||_2 r <= tolerance.
            let errorSquared = FloatingInterval.exact(0);
            let normSquared = FloatingInterval.exact(0);
            for (let i = 0; i < 3; ++i) {
                let mapped = FloatingInterval.exact(forward.get(i, 3));
                for (let j = 0; j < 3; ++j) {
                    mapped = mapped.add(matrix[i]![j]!.multiply(FloatingInterval.exact(center[j]!)));
                    normSquared = normSquared.add(matrix[i]![j]!.square());
                }
                mapped = mapped.subtract(FloatingInterval.exact(coordinates[i]!));
                errorSquared = errorSquared.add(mapped.square());
            }
            const error = errorSquared.squareRoot();
            // Any upper bound of the spectral norm works: use the smaller of the
            // Frobenius norm and sqrt(||A||_1 ||A||_inf).
            let norm = normSquared.squareRoot();
            let rowMaximum = FloatingInterval.exact(0),
                columnMaximum = FloatingInterval.exact(0);
            for (let i = 0; i < 3; ++i) {
                let row = FloatingInterval.exact(0),
                    column = FloatingInterval.exact(0);
                for (let j = 0; j < 3; ++j) {
                    row = row.add(FloatingInterval.exact(JavaMath.abs(forward.get(i, j))));
                    column = column.add(FloatingInterval.exact(JavaMath.abs(forward.get(j, i))));
                }
                if (row.isValid() && rowMaximum.isValid() && row.getUpper() > rowMaximum.getUpper()) rowMaximum = row;
                if (column.isValid() && columnMaximum.isValid() && column.getUpper() > columnMaximum.getUpper())
                    columnMaximum = column;
                if (!row.isValid()) rowMaximum = new FloatingInterval();
                if (!column.isValid()) columnMaximum = new FloatingInterval();
            }
            const mixed = rowMaximum.multiply(columnMaximum).squareRoot();
            if (mixed.isValid() && norm.isValid() && mixed.getUpper() < norm.getUpper() && mixed.getLower() > 0)
                norm = mixed;
            if (error.isValid() && norm.isValid() && norm.getLower() > 0) {
                const remaining = FloatingInterval.exact(tolerance).subtract(error);
                if (remaining.isValid() && remaining.getLower() > 0) {
                    const ratio = remaining.divide(norm);
                    if (ratio.isValid() && ratio.getLower() > 0) inner = ratio.getLower();
                }
            }
        }
        childPoint[0] = new Vector3Dd(center[0]!, center[1]!, center[2]!);
        childTolerance[0] = bound.getUpper();
        if (childInnerTolerance) childInnerTolerance[0] = inner;
        if (childHalfWidths) childHalfWidths[0] = new Vector3Dd(halfWidth[0]!, halfWidth[1]!, halfWidth[2]!);
        return AffinePlacementStatus.Valid;
    }
}
