#include "java/lang/Double.h"
#include "vsdk/toolkit/common/numerics/FloatingInterval.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"

GeometryBounds::GeometryBounds() : kind(Kind::Unsupported), minimum(), maximum() {}
void GeometryBounds::setUnsupported() { kind = Kind::Unsupported; minimum = Vector3Dd(); maximum = Vector3Dd(); }
void GeometryBounds::setEmpty() { kind = Kind::Empty; minimum = Vector3Dd(); maximum = Vector3Dd(); }
void GeometryBounds::setUnbounded() { kind = Kind::Unbounded; minimum = Vector3Dd(); maximum = Vector3Dd(); }
void GeometryBounds::setIndeterminate() { kind = Kind::Indeterminate; minimum = Vector3Dd(); maximum = Vector3Dd(); }
bool GeometryBounds::setFinite(const Vector3Dd& lo, const Vector3Dd& hi)
{
    if (!java::Double::isFinite(lo.x()) || !java::Double::isFinite(lo.y()) ||
        !java::Double::isFinite(lo.z()) || !java::Double::isFinite(hi.x()) ||
        !java::Double::isFinite(hi.y()) || !java::Double::isFinite(hi.z()) ||
        lo.x() > hi.x() || lo.y() > hi.y() || lo.z() > hi.z()) {
        setIndeterminate();
        return false;
    }
    minimum = lo; maximum = hi; kind = Kind::Finite; return true;
}
GeometryBounds::Kind GeometryBounds::getKind() const { return kind; }
const Vector3Dd& GeometryBounds::getMinimum() const { return minimum; }
const Vector3Dd& GeometryBounds::getMaximum() const { return maximum; }

GeometryBounds GeometryBounds::transformed(const Matrix4x4d& placement) const
{
    GeometryBounds result;
    if (kind == Kind::Empty) { result.setEmpty(); return result; }
    if (kind == Kind::Unsupported) return result;
    if (kind == Kind::Indeterminate) { result.setIndeterminate(); return result; }
    for (int row = 0; row < 4; ++row) for (int column = 0; column < 4; ++column) {
        if (!java::Double::isFinite(placement.get(row, column))) {
            result.setIndeterminate(); return result;
        }
    }
    if (placement.get(3,0) != 0 || placement.get(3,1) != 0 ||
        placement.get(3,2) != 0 || placement.get(3,3) != 1) {
        result.setIndeterminate(); return result;
    }
    if (kind == Kind::Unbounded) { result.setUnbounded(); return result; }
    double lo[3] = {0,0,0}, hi[3] = {0,0,0};
    bool first = true;
    for (int ix = 0; ix < 2; ++ix) for (int iy = 0; iy < 2; ++iy) for (int iz = 0; iz < 2; ++iz) {
        const double point[3] = {ix ? maximum.x() : minimum.x(),
            iy ? maximum.y() : minimum.y(), iz ? maximum.z() : minimum.z()};
        for (int row = 0; row < 3; ++row) {
            FloatingInterval value = FloatingInterval::exact(placement.get(row,3));
            for (int column = 0; column < 3; ++column)
                value = value.add(FloatingInterval::exact(placement.get(row,column))
                    .multiply(FloatingInterval::exact(point[column])));
            if (!value.isValid()) { result.setIndeterminate(); return result; }
            if (first || value.getLower() < lo[row]) lo[row] = value.getLower();
            if (first || value.getUpper() > hi[row]) hi[row] = value.getUpper();
        }
        first = false;
    }
    result.setFinite(Vector3Dd(lo[0],lo[1],lo[2]), Vector3Dd(hi[0],hi[1],hi[2]));
    return result;
}

GeometryBounds GeometryBounds::enclosing(const GeometryBounds& other) const
{
    GeometryBounds result;
    if (kind == Kind::Empty) return other;
    if (other.kind == Kind::Empty) return *this;
    if (kind == Kind::Indeterminate || other.kind == Kind::Indeterminate) {
        result.setIndeterminate(); return result;
    }
    if (kind == Kind::Unsupported || other.kind == Kind::Unsupported) return result;
    if (kind == Kind::Unbounded || other.kind == Kind::Unbounded) {
        result.setUnbounded(); return result;
    }
    result.setFinite(Vector3Dd(
        minimum.x() < other.minimum.x() ? minimum.x() : other.minimum.x(),
        minimum.y() < other.minimum.y() ? minimum.y() : other.minimum.y(),
        minimum.z() < other.minimum.z() ? minimum.z() : other.minimum.z()),
        Vector3Dd(maximum.x() > other.maximum.x() ? maximum.x() : other.maximum.x(),
        maximum.y() > other.maximum.y() ? maximum.y() : other.maximum.y(),
        maximum.z() > other.maximum.z() ? maximum.z() : other.maximum.z()));
    return result;
}

GeometryBounds GeometryBounds::intersecting(const GeometryBounds& other) const
{
    GeometryBounds result;
    if (kind == Kind::Empty || other.kind == Kind::Empty) { result.setEmpty(); return result; }
    const bool known = kind == Kind::Finite || kind == Kind::Unbounded;
    const bool otherKnown = other.kind == Kind::Finite || other.kind == Kind::Unbounded;
    if (!known && !otherKnown) {
        if (kind == Kind::Indeterminate || other.kind == Kind::Indeterminate) result.setIndeterminate();
        return result;
    }
    if (!known) return other;
    if (!otherKnown) return *this;
    if (kind == Kind::Unbounded) return other;
    if (other.kind == Kind::Unbounded) return *this;
    const double lo[3] = {minimum.x() > other.minimum.x() ? minimum.x() : other.minimum.x(),
        minimum.y() > other.minimum.y() ? minimum.y() : other.minimum.y(),
        minimum.z() > other.minimum.z() ? minimum.z() : other.minimum.z()};
    const double hi[3] = {maximum.x() < other.maximum.x() ? maximum.x() : other.maximum.x(),
        maximum.y() < other.maximum.y() ? maximum.y() : other.maximum.y(),
        maximum.z() < other.maximum.z() ? maximum.z() : other.maximum.z()};
    for (int i = 0; i < 3; ++i) if (lo[i] > hi[i]) { result.setEmpty(); return result; }
    result.setFinite(Vector3Dd(lo[0],lo[1],lo[2]), Vector3Dd(hi[0],hi[1],hi[2]));
    return result;
}

GeometryBounds GeometryBounds::inflated(double tolerance) const
{
    GeometryBounds result;
    if (!java::Double::isFinite(tolerance) || tolerance < 0) { result.setIndeterminate(); return result; }
    if (kind != Kind::Finite) return *this;
    const double lo[3] = {minimum.x(), minimum.y(), minimum.z()};
    const double hi[3] = {maximum.x(), maximum.y(), maximum.z()};
    double outLo[3], outHi[3];
    for (int i = 0; i < 3; ++i) {
        const FloatingInterval low = FloatingInterval::exact(lo[i]).subtract(FloatingInterval::exact(tolerance));
        const FloatingInterval high = FloatingInterval::exact(hi[i]).add(FloatingInterval::exact(tolerance));
        if (!low.isValid() || !high.isValid()) { result.setIndeterminate(); return result; }
        outLo[i] = tolerance == 0 ? lo[i] : low.getLower();
        outHi[i] = tolerance == 0 ? hi[i] : high.getUpper();
    }
    result.setFinite(Vector3Dd(outLo[0],outLo[1],outLo[2]), Vector3Dd(outHi[0],outHi[1],outHi[2]));
    return result;
}

bool GeometryBounds::excludesBall(const Vector3Dd& point, double tolerance) const
{
    if (!java::Double::isFinite(point.x()) || !java::Double::isFinite(point.y()) ||
        !java::Double::isFinite(point.z()) || !java::Double::isFinite(tolerance) || tolerance < 0)
        return false;
    if (kind == Kind::Empty) return true;
    if (kind != Kind::Finite) return false;
    const double p[3] = {point.x(), point.y(), point.z()};
    const double lo[3] = {minimum.x(), minimum.y(), minimum.z()};
    const double hi[3] = {maximum.x(), maximum.y(), maximum.z()};
    FloatingInterval squared = FloatingInterval::exact(0);
    bool outside = false;
    for (int i = 0; i < 3; ++i) {
        if (p[i] < lo[i]) { outside = true; squared = squared.add(FloatingInterval::exact(lo[i]).subtract(FloatingInterval::exact(p[i])).square()); }
        else if (p[i] > hi[i]) { outside = true; squared = squared.add(FloatingInterval::exact(p[i]).subtract(FloatingInterval::exact(hi[i])).square()); }
    }
    if (!outside) return false;
    const FloatingInterval distance = squared.squareRoot();
    return distance.isValid() && distance.getLower() > tolerance;
}
