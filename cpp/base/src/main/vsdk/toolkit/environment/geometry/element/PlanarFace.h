#ifndef __PLANAR_FACE__
#define __PLANAR_FACE__

#include "vsdk/toolkit/common/linealAlgebra/Vector2Dd.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"

/**
A planar triangle or quadrilateral in double precision, as used by the
radiosity solvers: the face of a polygonal mesh with its plane, area,
parametrization and uniform (area preserving) sampling.

The face copies its three or four vertex positions when it is built and
never changes afterwards, so every operation is const and a face can be
shared by concurrent readers.

Every quantity is computed in double precision, in source operation order and
with IEEE semantics: PlanarFace.cpp is compiled without fast-math and without
floating point contraction, also inside applications built with fast-math,
so results are reproducible bit for bit across builds. For that reason no
arithmetic is inline in this header; do not rewrite it with vector helpers.

Parametrizations:
- triangle: barycentric, point = p0 + u (p1 - p0) + v (p2 - p0)
- quadrilateral: bi-linear, point = p0 + (u - uv) (p1 - p0) + uv (p2 - p0)
  + (v - uv) (p3 - p0)
The Jacobian of the quadrilateral mapping is J(u, v) = A + B u + C v; for
triangles and parallelograms it is constant and equal to the area, and the
face reports that it has no Jacobian.
*/
class PlanarFace {
  public:
    static const int MAXIMUM_VERTICES = 4;
    static const double EPSILON;

    enum class BuildStatus {
        VALID = 0,
        UNSUPPORTED_VERTEX_COUNT = 1,
        DEGENERATE_NORMAL = 2
    };

    enum class Side {
        PARALLEL = 0,
        FRONT = 1,
        BACK = 2
    };

    // Result of rayHit: the side the ray arrives at, the ray parameter of the
    // plane hit, the hit point and its (u, v) parameters. rayHit fills it
    // through the setters
    class RayHit {
      private:
        Side side;
        double distance;
        double pointX;
        double pointY;
        double pointZ;
        double u;
        double v;
      public:
        RayHit() : side(Side::PARALLEL), distance(0.0), pointX(0.0), pointY(0.0), pointZ(0.0), u(0.0), v(0.0) {}
        Side getSide() const { return side; }
        double getDistance() const { return distance; }
        double getPointX() const { return pointX; }
        double getPointY() const { return pointY; }
        double getPointZ() const { return pointZ; }
        double getU() const { return u; }
        double getV() const { return v; }
        void setSide(const Side inSide) { side = inSide; }
        void setDistance(const double inDistance) { distance = inDistance; }
        void setPoint(const double x, const double y, const double z) { pointX = x; pointY = y; pointZ = z; }
        void setParameters(const double inU, const double inV) { u = inU; v = inV; }
    };

  private:
    static constexpr double TOLERANCE = 1e-5;

    // Coordinates kept as plain doubles so that the face is trivially copyable
    double positionX[MAXIMUM_VERTICES];
    double positionY[MAXIMUM_VERTICES];
    double positionZ[MAXIMUM_VERTICES];
    double normalX;
    double normalY;
    double normalZ;
    double midPointX;
    double midPointY;
    double midPointZ;
    double planeConstant;
    double tolerance;
    double area;
    double jacobianA;
    double jacobianB;
    double jacobianC;
    int numberOfVertices;
    int dominantAxis; // 0: x, 1: y, 2: z
    bool jacobianPresent;
    BuildStatus status;

    bool computeNormal();
    void computeAreaAndJacobian();
    void computeMidPoint();
    double computeTolerance() const;
    static int dominantCoordinate(const Vector3Dd &vector);
    static double clipToUnitInterval(double x);
    static bool solveQuadraticUnitInterval(double A, double B, double C, double *x);
    bool triangleParameters(const Vector3Dd &point, Vector2Dd *uv) const;
    bool quadrilateralParameters(const Vector3Dd &point, Vector2Dd *uv) const;
    void projectedPosition(int i, double *a, double *b) const;

  public:
    PlanarFace();
    PlanarFace(int inNumberOfVertices, const Vector3Dd *inPositions);

    BuildStatus getStatus() const;
    bool isValid() const;
    int getNumberOfVertices() const;
    Vector3Dd getPosition(int i) const;
    Vector3Dd getNormal() const;
    Vector3Dd getMidPoint() const;
    double getPlaneConstant() const;
    double getTolerance() const;
    double getArea() const;
    int getDominantAxis() const;
    bool hasJacobian() const;
    double getJacobianA() const;
    double getJacobianB() const;
    double getJacobianC() const;

    // Signed distance of a point to the plane, in units of the normal
    double planeDistance(const Vector3Dd &point) const;

    // Side of the face a ray with the given direction arrives at: FRONT when
    // it travels against the normal, PARALLEL within EPSILON
    Side sideFacing(const Vector3Dd &direction, double *normalDotDirection) const;

    // Ray parameter of the hit with the plane, given the dot product of the
    // normal with the ray direction (from sideFacing)
    double planeHitParameter(const Vector3Dd &origin, double normalDotDirection) const;

    // One ray against the face, in one call: sideFacing, planeHitParameter,
    // the hit point origin + distance * direction and its parameters. Returns
    // true, with the hit stored, when the ray arrives at an accepted side, the
    // distance lies in [minimumDistance, maximumDistance] and the point is
    // inside the face; returns false otherwise (the hit is then unspecified)
    bool rayHit(
        const Vector3Dd &origin,
        const Vector3Dd &direction,
        bool acceptFront,
        bool acceptBack,
        double minimumDistance,
        double maximumDistance,
        RayHit *hit) const;

    // (u, v) of a point on the plane of the face; returns whether the point is
    // inside. For points outside, (u, v) is not meaningful
    bool parameters(const Vector3Dd &point, double *u, double *v) const;

    // Point at the given barycentric (triangle) or bi-linear (quadrilateral)
    // parameters; for a triangle with u + v > 1, (1 - u, 1 - v) is used
    Vector3Dd pointAt(double u, double v) const;

    // Area preserving mapping: uniform parameters to point, point to uniform
    // parameters, and the conversions between uniform and bi-linear parameters
    Vector3Dd uniformPointAt(double u, double v) const;
    bool uniformParameters(const Vector3Dd &point, double *u, double *v) const;
    void biLinearToUniform(double *u, double *v) const;
    void uniformToBiLinear(double *u, double *v) const;

    // True when part of this face lies in front of the plane of the other face,
    // beyond the other face's tolerance
    bool isAtLeastPartlyInFront(const PlanarFace &other) const;

    // True when each face is at least partly in front of the other
    bool facing(const PlanarFace &other) const;
};

inline PlanarFace::BuildStatus
PlanarFace::getStatus() const {
    return status;
}

inline bool
PlanarFace::isValid() const {
    return status == BuildStatus::VALID;
}

inline int
PlanarFace::getNumberOfVertices() const {
    return numberOfVertices;
}

inline Vector3Dd
PlanarFace::getPosition(const int i) const {
    return Vector3Dd(positionX[i], positionY[i], positionZ[i]);
}

inline Vector3Dd
PlanarFace::getNormal() const {
    return Vector3Dd(normalX, normalY, normalZ);
}

inline Vector3Dd
PlanarFace::getMidPoint() const {
    return Vector3Dd(midPointX, midPointY, midPointZ);
}

inline double
PlanarFace::getPlaneConstant() const {
    return planeConstant;
}

inline double
PlanarFace::getTolerance() const {
    return tolerance;
}

inline double
PlanarFace::getArea() const {
    return area;
}

inline int
PlanarFace::getDominantAxis() const {
    return dominantAxis;
}

inline bool
PlanarFace::hasJacobian() const {
    return jacobianPresent;
}

inline double
PlanarFace::getJacobianA() const {
    return jacobianA;
}

inline double
PlanarFace::getJacobianB() const {
    return jacobianB;
}

inline double
PlanarFace::getJacobianC() const {
    return jacobianC;
}

#endif
