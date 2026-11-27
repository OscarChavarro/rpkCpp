import { Double } from "../../../../../java/lang/Double.js";
import type { Matrix4x4d } from "../../../common/linealAlgebra/Matrix4x4d.js";
import { Vector3Dd } from "../../../common/linealAlgebra/Vector3Dd.js";
import { FloatingInterval } from "../../../common/numerics/FloatingInterval.js";

// Values are owned by this result, in the geometry's local coordinates.
export class GeometryBounds {
    private kind: GeometryBounds.Kind = GeometryBounds.Kind.Unsupported;
    private minimum = new Vector3Dd();
    private maximum = new Vector3Dd();

    public constructor();
    /** C++ copy constructor. */
    public constructor(other: GeometryBounds);
    public constructor(other?: GeometryBounds) {
        if (other !== undefined) this.assign(other);
    }
    /** C++ copy assignment; vectors are immutable in this port. */
    public assign(other: GeometryBounds): GeometryBounds {
        this.kind = other.kind;
        this.minimum = other.minimum;
        this.maximum = other.maximum;
        return this;
    }
    public setUnsupported(): void {
        this.kind = GeometryBounds.Kind.Unsupported;
        this.minimum = new Vector3Dd();
        this.maximum = new Vector3Dd();
    }
    public setEmpty(): void {
        this.kind = GeometryBounds.Kind.Empty;
        this.minimum = new Vector3Dd();
        this.maximum = new Vector3Dd();
    }
    public setUnbounded(): void {
        this.kind = GeometryBounds.Kind.Unbounded;
        this.minimum = new Vector3Dd();
        this.maximum = new Vector3Dd();
    }
    public setIndeterminate(): void {
        this.kind = GeometryBounds.Kind.Indeterminate;
        this.minimum = new Vector3Dd();
        this.maximum = new Vector3Dd();
    }
    public setFinite(lo: Vector3Dd, hi: Vector3Dd): boolean {
        if (
            !Double.isFinite(lo.x()) ||
            !Double.isFinite(lo.y()) ||
            !Double.isFinite(lo.z()) ||
            !Double.isFinite(hi.x()) ||
            !Double.isFinite(hi.y()) ||
            !Double.isFinite(hi.z()) ||
            lo.x() > hi.x() ||
            lo.y() > hi.y() ||
            lo.z() > hi.z()
        ) {
            this.setIndeterminate();
            return false;
        }
        this.minimum = lo;
        this.maximum = hi;
        this.kind = GeometryBounds.Kind.Finite;
        return true;
    }
    public getKind(): GeometryBounds.Kind {
        return this.kind;
    }
    public getMinimum(): Vector3Dd {
        return this.minimum;
    }
    public getMaximum(): Vector3Dd {
        return this.maximum;
    }

    // Conservative axis-aligned enclosure in the destination coordinates,
    // computed with outward rounding. Non-affine or numerically invalid
    // transforms are indeterminate.
    public transformed(placement: Matrix4x4d): GeometryBounds {
        const result = new GeometryBounds();
        if (this.kind === GeometryBounds.Kind.Empty) {
            result.setEmpty();
            return result;
        }
        if (this.kind === GeometryBounds.Kind.Unsupported) return result;
        if (this.kind === GeometryBounds.Kind.Indeterminate) {
            result.setIndeterminate();
            return result;
        }
        for (let row = 0; row < 4; ++row)
            for (let column = 0; column < 4; ++column) {
                if (!Double.isFinite(placement.get(row, column))) {
                    result.setIndeterminate();
                    return result;
                }
            }
        if (
            placement.get(3, 0) !== 0 ||
            placement.get(3, 1) !== 0 ||
            placement.get(3, 2) !== 0 ||
            placement.get(3, 3) !== 1
        ) {
            result.setIndeterminate();
            return result;
        }
        if (this.kind === GeometryBounds.Kind.Unbounded) {
            result.setUnbounded();
            return result;
        }
        const lo = [0, 0, 0],
            hi = [0, 0, 0];
        let first = true;
        for (let ix = 0; ix < 2; ++ix)
            for (let iy = 0; iy < 2; ++iy)
                for (let iz = 0; iz < 2; ++iz) {
                    const point = [
                        ix ? this.maximum.x() : this.minimum.x(),
                        iy ? this.maximum.y() : this.minimum.y(),
                        iz ? this.maximum.z() : this.minimum.z(),
                    ];
                    for (let row = 0; row < 3; ++row) {
                        let value = FloatingInterval.exact(placement.get(row, 3));
                        for (let column = 0; column < 3; ++column)
                            value = value.add(
                                FloatingInterval.exact(placement.get(row, column)).multiply(
                                    FloatingInterval.exact(point[column]!),
                                ),
                            );
                        if (!value.isValid()) {
                            result.setIndeterminate();
                            return result;
                        }
                        if (first || value.getLower() < lo[row]!) lo[row] = value.getLower();
                        if (first || value.getUpper() > hi[row]!) hi[row] = value.getUpper();
                    }
                    first = false;
                }
        result.setFinite(new Vector3Dd(lo[0]!, lo[1]!, lo[2]!), new Vector3Dd(hi[0]!, hi[1]!, hi[2]!));
        return result;
    }

    // Enclose both bounds; unknown states remain explicit.
    public enclosing(other: GeometryBounds): GeometryBounds {
        const result = new GeometryBounds();
        if (this.kind === GeometryBounds.Kind.Empty) return new GeometryBounds(other);
        if (other.kind === GeometryBounds.Kind.Empty) return new GeometryBounds(this);
        if (this.kind === GeometryBounds.Kind.Indeterminate || other.kind === GeometryBounds.Kind.Indeterminate) {
            result.setIndeterminate();
            return result;
        }
        if (this.kind === GeometryBounds.Kind.Unsupported || other.kind === GeometryBounds.Kind.Unsupported)
            return result;
        if (this.kind === GeometryBounds.Kind.Unbounded || other.kind === GeometryBounds.Kind.Unbounded) {
            result.setUnbounded();
            return result;
        }
        const minimum = this.minimum,
            maximum = this.maximum;
        result.setFinite(
            new Vector3Dd(
                minimum.x() < other.minimum.x() ? minimum.x() : other.minimum.x(),
                minimum.y() < other.minimum.y() ? minimum.y() : other.minimum.y(),
                minimum.z() < other.minimum.z() ? minimum.z() : other.minimum.z(),
            ),
            new Vector3Dd(
                maximum.x() > other.maximum.x() ? maximum.x() : other.maximum.x(),
                maximum.y() > other.maximum.y() ? maximum.y() : other.maximum.y(),
                maximum.z() > other.maximum.z() ? maximum.z() : other.maximum.z(),
            ),
        );
        return result;
    }

    // Enclose the intersection. A known operand alone still encloses it, so an
    // unknown operand only remains when neither side is known.
    public intersecting(other: GeometryBounds): GeometryBounds {
        const result = new GeometryBounds();
        if (this.kind === GeometryBounds.Kind.Empty || other.kind === GeometryBounds.Kind.Empty) {
            result.setEmpty();
            return result;
        }
        const known = this.kind === GeometryBounds.Kind.Finite || this.kind === GeometryBounds.Kind.Unbounded;
        const otherKnown = other.kind === GeometryBounds.Kind.Finite || other.kind === GeometryBounds.Kind.Unbounded;
        if (!known && !otherKnown) {
            if (this.kind === GeometryBounds.Kind.Indeterminate || other.kind === GeometryBounds.Kind.Indeterminate)
                result.setIndeterminate();
            return result;
        }
        if (!known) return new GeometryBounds(other);
        if (!otherKnown) return new GeometryBounds(this);
        if (this.kind === GeometryBounds.Kind.Unbounded) return new GeometryBounds(other);
        if (other.kind === GeometryBounds.Kind.Unbounded) return new GeometryBounds(this);
        const minimum = this.minimum,
            maximum = this.maximum;
        const lo = [
            minimum.x() > other.minimum.x() ? minimum.x() : other.minimum.x(),
            minimum.y() > other.minimum.y() ? minimum.y() : other.minimum.y(),
            minimum.z() > other.minimum.z() ? minimum.z() : other.minimum.z(),
        ];
        const hi = [
            maximum.x() < other.maximum.x() ? maximum.x() : other.maximum.x(),
            maximum.y() < other.maximum.y() ? maximum.y() : other.maximum.y(),
            maximum.z() < other.maximum.z() ? maximum.z() : other.maximum.z(),
        ];
        for (let i = 0; i < 3; ++i)
            if (lo[i]! > hi[i]!) {
                result.setEmpty();
                return result;
            }
        result.setFinite(new Vector3Dd(lo[0]!, lo[1]!, lo[2]!), new Vector3Dd(hi[0]!, hi[1]!, hi[2]!));
        return result;
    }

    // Enclose all points within the world tolerance of these bounds.
    public inflated(tolerance: number): GeometryBounds {
        const result = new GeometryBounds();
        if (!Double.isFinite(tolerance) || tolerance < 0) {
            result.setIndeterminate();
            return result;
        }
        if (this.kind !== GeometryBounds.Kind.Finite) return new GeometryBounds(this);
        const lo = [this.minimum.x(), this.minimum.y(), this.minimum.z()];
        const hi = [this.maximum.x(), this.maximum.y(), this.maximum.z()];
        const outLo = [0, 0, 0],
            outHi = [0, 0, 0];
        for (let i = 0; i < 3; ++i) {
            const low = FloatingInterval.exact(lo[i]!).subtract(FloatingInterval.exact(tolerance));
            const high = FloatingInterval.exact(hi[i]!).add(FloatingInterval.exact(tolerance));
            if (!low.isValid() || !high.isValid()) {
                result.setIndeterminate();
                return result;
            }
            outLo[i] = tolerance === 0 ? lo[i]! : low.getLower();
            outHi[i] = tolerance === 0 ? hi[i]! : high.getUpper();
        }
        result.setFinite(
            new Vector3Dd(outLo[0]!, outLo[1]!, outLo[2]!),
            new Vector3Dd(outHi[0]!, outHi[1]!, outHi[2]!),
        );
        return result;
    }

    // True only when the closed ball is proven disjoint from these bounds.
    // Unknown and unbounded kinds never exclude anything.
    public excludesBall(point: Vector3Dd, tolerance: number): boolean {
        if (
            !Double.isFinite(point.x()) ||
            !Double.isFinite(point.y()) ||
            !Double.isFinite(point.z()) ||
            !Double.isFinite(tolerance) ||
            tolerance < 0
        )
            return false;
        if (this.kind === GeometryBounds.Kind.Empty) return true;
        if (this.kind !== GeometryBounds.Kind.Finite) return false;
        const p = [point.x(), point.y(), point.z()];
        const lo = [this.minimum.x(), this.minimum.y(), this.minimum.z()];
        const hi = [this.maximum.x(), this.maximum.y(), this.maximum.z()];
        let squared = FloatingInterval.exact(0);
        let outside = false;
        for (let i = 0; i < 3; ++i) {
            if (p[i]! < lo[i]!) {
                outside = true;
                squared = squared.add(FloatingInterval.exact(lo[i]!).subtract(FloatingInterval.exact(p[i]!)).square());
            } else if (p[i]! > hi[i]!) {
                outside = true;
                squared = squared.add(FloatingInterval.exact(p[i]!).subtract(FloatingInterval.exact(hi[i]!)).square());
            }
        }
        if (!outside) return false;
        const distance = squared.squareRoot();
        return distance.isValid() && distance.getLower() > tolerance;
    }
}

export namespace GeometryBounds {
    export enum Kind {
        Unsupported,
        Empty,
        Finite,
        Unbounded,
        Indeterminate,
    }
}
