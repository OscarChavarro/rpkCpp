#ifndef __GEOMETRY_BOUNDS__
#define __GEOMETRY_BOUNDS__

#include "vsdk/toolkit/common/VSDK.h"

#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

class Matrix4x4d;

// Values are owned by this result, in the geometry's local coordinates.
class GeometryBounds {
public:
    class Kind { public: enum Value { Unsupported, Empty, Finite, Unbounded, Indeterminate }; };
private:
    Kind::Value kind;
    Vector3Dd minimum;
    Vector3Dd maximum;
public:
    GeometryBounds();
    void setUnsupported();
    void setEmpty();
    void setUnbounded();
    void setIndeterminate();
    BOOLEAN setFinite(const Vector3Dd& minimum, const Vector3Dd& maximum);
    Kind::Value getKind() const;
    const Vector3Dd& getMinimum() const;
    const Vector3Dd& getMaximum() const;
    // Conservative axis-aligned enclosure in the destination coordinates,
    // computed with outward rounding. Non-affine or numerically invalid
    // transforms are indeterminate.
    GeometryBounds transformed(const Matrix4x4d& placement) const;
    // Enclose both bounds; unknown states remain explicit.
    GeometryBounds enclosing(const GeometryBounds& other) const;
    // Enclose the intersection. A known operand alone still encloses it, so an
    // unknown operand only remains when neither side is known.
    GeometryBounds intersecting(const GeometryBounds& other) const;
    // Enclose all points within the world tolerance of these bounds.
    GeometryBounds inflated(double tolerance) const;
    // True only when the closed ball is proven disjoint from these bounds.
    // Unknown and unbounded kinds never exclude anything.
    BOOLEAN excludesBall(const Vector3Dd& point, double tolerance) const;
};
#endif
