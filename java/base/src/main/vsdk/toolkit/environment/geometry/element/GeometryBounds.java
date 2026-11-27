package vsdk.toolkit.environment.geometry.element;

import vsdk.toolkit.common.linealAlgebra.Matrix4x4d;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.numerics.FloatingInterval;

/** Values are owned by this result, in the geometry's local coordinates. */
public class GeometryBounds {
    public enum Kind { Unsupported, Empty, Finite, Unbounded, Indeterminate }

    private Kind kind;
    private Vector3Dd minimum;
    private Vector3Dd maximum;

    public GeometryBounds() { kind = Kind.Unsupported; minimum = new Vector3Dd(); maximum = new Vector3Dd(); }
    /** cpp value copy. */
    public GeometryBounds(GeometryBounds other) { kind = other.kind; minimum = other.minimum; maximum = other.maximum; }
    /** cpp copy assignment. */
    public GeometryBounds assign(GeometryBounds other) { kind = other.kind; minimum = other.minimum; maximum = other.maximum; return this; }

    public void setUnsupported() { kind = Kind.Unsupported; minimum = new Vector3Dd(); maximum = new Vector3Dd(); }
    public void setEmpty() { kind = Kind.Empty; minimum = new Vector3Dd(); maximum = new Vector3Dd(); }
    public void setUnbounded() { kind = Kind.Unbounded; minimum = new Vector3Dd(); maximum = new Vector3Dd(); }
    public void setIndeterminate() { kind = Kind.Indeterminate; minimum = new Vector3Dd(); maximum = new Vector3Dd(); }
    public boolean setFinite(Vector3Dd lo, Vector3Dd hi)
    {
        if (!Double.isFinite(lo.x()) || !Double.isFinite(lo.y()) ||
            !Double.isFinite(lo.z()) || !Double.isFinite(hi.x()) ||
            !Double.isFinite(hi.y()) || !Double.isFinite(hi.z()) ||
            lo.x() > hi.x() || lo.y() > hi.y() || lo.z() > hi.z()) {
            setIndeterminate();
            return false;
        }
        minimum = lo; maximum = hi; kind = Kind.Finite; return true;
    }
    public Kind getKind() { return kind; }
    public Vector3Dd getMinimum() { return minimum; }
    public Vector3Dd getMaximum() { return maximum; }

    /**
    Conservative axis-aligned enclosure in the destination coordinates,
    computed with outward rounding. Non-affine or numerically invalid
    transforms are indeterminate.
    */
    public GeometryBounds transformed(Matrix4x4d placement)
    {
        GeometryBounds result = new GeometryBounds();
        if (kind == Kind.Empty) { result.setEmpty(); return result; }
        if (kind == Kind.Unsupported) return result;
        if (kind == Kind.Indeterminate) { result.setIndeterminate(); return result; }
        for (int row = 0; row < 4; ++row) for (int column = 0; column < 4; ++column) {
            if (!Double.isFinite(placement.get(row, column))) {
                result.setIndeterminate(); return result;
            }
        }
        if (placement.get(3,0) != 0 || placement.get(3,1) != 0 ||
            placement.get(3,2) != 0 || placement.get(3,3) != 1) {
            result.setIndeterminate(); return result;
        }
        if (kind == Kind.Unbounded) { result.setUnbounded(); return result; }
        double[] lo = {0,0,0}, hi = {0,0,0};
        boolean first = true;
        for (int ix = 0; ix < 2; ++ix) for (int iy = 0; iy < 2; ++iy) for (int iz = 0; iz < 2; ++iz) {
            final double[] point = {ix != 0 ? maximum.x() : minimum.x(),
                iy != 0 ? maximum.y() : minimum.y(), iz != 0 ? maximum.z() : minimum.z()};
            for (int row = 0; row < 3; ++row) {
                FloatingInterval value = FloatingInterval.exact(placement.get(row,3));
                for (int column = 0; column < 3; ++column)
                    value = value.add(FloatingInterval.exact(placement.get(row,column))
                        .multiply(FloatingInterval.exact(point[column])));
                if (!value.isValid()) { result.setIndeterminate(); return result; }
                if (first || value.getLower() < lo[row]) lo[row] = value.getLower();
                if (first || value.getUpper() > hi[row]) hi[row] = value.getUpper();
            }
            first = false;
        }
        result.setFinite(new Vector3Dd(lo[0],lo[1],lo[2]), new Vector3Dd(hi[0],hi[1],hi[2]));
        return result;
    }

    /** Enclose both bounds; unknown states remain explicit. */
    public GeometryBounds enclosing(GeometryBounds other)
    {
        GeometryBounds result = new GeometryBounds();
        if (kind == Kind.Empty) return new GeometryBounds(other);
        if (other.kind == Kind.Empty) return new GeometryBounds(this);
        if (kind == Kind.Indeterminate || other.kind == Kind.Indeterminate) {
            result.setIndeterminate(); return result;
        }
        if (kind == Kind.Unsupported || other.kind == Kind.Unsupported) return result;
        if (kind == Kind.Unbounded || other.kind == Kind.Unbounded) {
            result.setUnbounded(); return result;
        }
        result.setFinite(new Vector3Dd(
            minimum.x() < other.minimum.x() ? minimum.x() : other.minimum.x(),
            minimum.y() < other.minimum.y() ? minimum.y() : other.minimum.y(),
            minimum.z() < other.minimum.z() ? minimum.z() : other.minimum.z()),
            new Vector3Dd(maximum.x() > other.maximum.x() ? maximum.x() : other.maximum.x(),
            maximum.y() > other.maximum.y() ? maximum.y() : other.maximum.y(),
            maximum.z() > other.maximum.z() ? maximum.z() : other.maximum.z()));
        return result;
    }

    /**
    Enclose the intersection. A known operand alone still encloses it, so an
    unknown operand only remains when neither side is known.
    */
    public GeometryBounds intersecting(GeometryBounds other)
    {
        GeometryBounds result = new GeometryBounds();
        if (kind == Kind.Empty || other.kind == Kind.Empty) { result.setEmpty(); return result; }
        final boolean known = kind == Kind.Finite || kind == Kind.Unbounded;
        final boolean otherKnown = other.kind == Kind.Finite || other.kind == Kind.Unbounded;
        if (!known && !otherKnown) {
            if (kind == Kind.Indeterminate || other.kind == Kind.Indeterminate) result.setIndeterminate();
            return result;
        }
        if (!known) return new GeometryBounds(other);
        if (!otherKnown) return new GeometryBounds(this);
        if (kind == Kind.Unbounded) return new GeometryBounds(other);
        if (other.kind == Kind.Unbounded) return new GeometryBounds(this);
        final double[] lo = {minimum.x() > other.minimum.x() ? minimum.x() : other.minimum.x(),
            minimum.y() > other.minimum.y() ? minimum.y() : other.minimum.y(),
            minimum.z() > other.minimum.z() ? minimum.z() : other.minimum.z()};
        final double[] hi = {maximum.x() < other.maximum.x() ? maximum.x() : other.maximum.x(),
            maximum.y() < other.maximum.y() ? maximum.y() : other.maximum.y(),
            maximum.z() < other.maximum.z() ? maximum.z() : other.maximum.z()};
        for (int i = 0; i < 3; ++i) if (lo[i] > hi[i]) { result.setEmpty(); return result; }
        result.setFinite(new Vector3Dd(lo[0],lo[1],lo[2]), new Vector3Dd(hi[0],hi[1],hi[2]));
        return result;
    }

    /** Enclose all points within the world tolerance of these bounds. */
    public GeometryBounds inflated(double tolerance)
    {
        GeometryBounds result = new GeometryBounds();
        if (!Double.isFinite(tolerance) || tolerance < 0) { result.setIndeterminate(); return result; }
        if (kind != Kind.Finite) return new GeometryBounds(this);
        final double[] lo = {minimum.x(), minimum.y(), minimum.z()};
        final double[] hi = {maximum.x(), maximum.y(), maximum.z()};
        double[] outLo = new double[3], outHi = new double[3];
        for (int i = 0; i < 3; ++i) {
            final FloatingInterval low = FloatingInterval.exact(lo[i]).subtract(FloatingInterval.exact(tolerance));
            final FloatingInterval high = FloatingInterval.exact(hi[i]).add(FloatingInterval.exact(tolerance));
            if (!low.isValid() || !high.isValid()) { result.setIndeterminate(); return result; }
            outLo[i] = tolerance == 0 ? lo[i] : low.getLower();
            outHi[i] = tolerance == 0 ? hi[i] : high.getUpper();
        }
        result.setFinite(new Vector3Dd(outLo[0],outLo[1],outLo[2]), new Vector3Dd(outHi[0],outHi[1],outHi[2]));
        return result;
    }

    /**
    True only when the closed ball is proven disjoint from these bounds.
    Unknown and unbounded kinds never exclude anything.
    */
    public boolean excludesBall(Vector3Dd point, double tolerance)
    {
        if (!Double.isFinite(point.x()) || !Double.isFinite(point.y()) ||
            !Double.isFinite(point.z()) || !Double.isFinite(tolerance) || tolerance < 0)
            return false;
        if (kind == Kind.Empty) return true;
        if (kind != Kind.Finite) return false;
        final double[] p = {point.x(), point.y(), point.z()};
        final double[] lo = {minimum.x(), minimum.y(), minimum.z()};
        final double[] hi = {maximum.x(), maximum.y(), maximum.z()};
        FloatingInterval squared = FloatingInterval.exact(0);
        boolean outside = false;
        for (int i = 0; i < 3; ++i) {
            if (p[i] < lo[i]) { outside = true; squared = squared.add(FloatingInterval.exact(lo[i]).subtract(FloatingInterval.exact(p[i])).square()); }
            else if (p[i] > hi[i]) { outside = true; squared = squared.add(FloatingInterval.exact(p[i]).subtract(FloatingInterval.exact(hi[i])).square()); }
        }
        if (!outside) return false;
        final FloatingInterval distance = squared.squareRoot();
        return distance.isValid() && distance.getLower() > tolerance;
    }
}
