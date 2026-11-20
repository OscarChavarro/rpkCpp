#include "java/lang/Math.h"
#include "java/lang/String.h"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/environment/geometry/element/PlanarFace.h"

const double PlanarFace::EPSILON = 1e-6;
const float PlanarFace::EPSILON_FLOAT = 1e-6F;

namespace {

// Single precision helpers, written out so that the operation order is the
// one documented by PlanarFace and does not depend on Vector3Df's methods

inline Vector3Df
difference(const Vector3Df &a, const Vector3Df &b) {
    return Vector3Df(a.x() - b.x(), a.y() - b.y(), a.z() - b.z());
}

inline Vector3Df
cross(const Vector3Df &a, const Vector3Df &b) {
    return Vector3Df(
        a.y() * b.z() - a.z() * b.y(),
        a.z() * b.x() - a.x() * b.z(),
        a.x() * b.y() - a.y() * b.x());
}

inline float
dot(const Vector3Df &a, const Vector3Df &b) {
    return a.x() * b.x() + a.y() * b.y() + a.z() * b.z();
}

inline float
norm(const Vector3Df &a) {
    return java::Math::sqrt(a.x() * a.x() + a.y() * a.y() + a.z() * a.z());
}

// (1 / s) v, or v when s is within epsilon of zero
inline Vector3Df
inverseScaled(const float s, const Vector3Df &v, const float epsilon) {
    const float factor = s < -epsilon || s > epsilon ? 1.0F / s : 1.0F;
    return Vector3Df(factor * v.x(), factor * v.y(), factor * v.z());
}

inline float
positionTolerance(const Vector3Df &p, const float epsilon) {
    return epsilon * (java::Math::abs(p.x()) + java::Math::abs(p.y()) + java::Math::abs(p.z()));
}

inline double
determinant(const Vector2Dd &a, const Vector2Dd &b) {
    return a.x * b.y - a.y * b.x;
}

inline Vector3Df
pointInTriangle(const Vector3Df &v0, const Vector3Df &v1, const Vector3Df &v2, const float u, const float v) {
    return Vector3Df(
        v0.x() + u * (v1.x() - v0.x()) + v * (v2.x() - v0.x()),
        v0.y() + u * (v1.y() - v0.y()) + v * (v2.y() - v0.y()),
        v0.z() + u * (v1.z() - v0.z()) + v * (v2.z() - v0.z()));
}

inline Vector3Df
pointInQuadrilateral(
    const Vector3Df &v0,
    const Vector3Df &v1,
    const Vector3Df &v2,
    const Vector3Df &v3,
    const float u,
    const float v)
{
    const float c = u * v;
    const float b = u - c;
    const float d = v - c;
    return Vector3Df(
        v0.x() + b * (v1.x() - v0.x()) + c * (v2.x() - v0.x()) + d * (v3.x() - v0.x()),
        v0.y() + b * (v1.y() - v0.y()) + c * (v2.y() - v0.y()) + d * (v3.y() - v0.y()),
        v0.z() + b * (v1.z() - v0.z()) + c * (v2.z() - v0.z()) + d * (v3.z() - v0.z()));
}

}

PlanarFace::PlanarFace():
    positionX(),
    positionY(),
    positionZ(),
    normalX(),
    normalY(),
    normalZ(),
    midPointX(),
    midPointY(),
    midPointZ(),
    planeConstant(),
    tolerance(),
    area(),
    jacobianA(),
    jacobianB(),
    jacobianC(),
    numberOfVertices(),
    dominantAxis(),
    jacobianPresent(),
    status(BuildStatus::UNSUPPORTED_VERTEX_COUNT)
{
}

/**
Builds the face of the given three or four positions, in order. The face is
invalid (see getStatus) for any other vertex count or when the polygon is
degenerate (its normal cannot be computed)
*/
PlanarFace::PlanarFace(const int inNumberOfVertices, const Vector3Df *inPositions):
    PlanarFace()
{
    if ( inNumberOfVertices != 3 && inNumberOfVertices != 4 ) {
        status = BuildStatus::UNSUPPORTED_VERTEX_COUNT;
        return;
    }
    numberOfVertices = inNumberOfVertices;
    for ( int i = 0; i < numberOfVertices; i++ ) {
        positionX[i] = inPositions[i].x();
        positionY[i] = inPositions[i].y();
        positionZ[i] = inPositions[i].z();
    }

    if ( !computeNormal() ) {
        status = BuildStatus::DEGENERATE_NORMAL;
        return;
    }
    computeAreaAndJacobian();
    computeMidPoint();
    planeConstant = -dot(getNormal(), getMidPoint());
    tolerance = computeTolerance();
    dominantAxis = dominantCoordinate(getNormal());
    status = BuildStatus::VALID;
}

/**
Newell's method; false for a degenerate polygon
*/
bool
PlanarFace::computeNormal() {
    float nx = 0.0F;
    float ny = 0.0F;
    float nz = 0.0F;
    const int last = numberOfVertices - 1;
    float currentX = positionX[last] - positionX[0];
    float currentY = positionY[last] - positionY[0];
    float currentZ = positionZ[last] - positionZ[0];
    for ( int i = 0; i < numberOfVertices; i++ ) {
        const float previousX = currentX;
        const float previousY = currentY;
        const float previousZ = currentZ;
        currentX = positionX[i] - positionX[0];
        currentY = positionY[i] - positionY[0];
        currentZ = positionZ[i] - positionZ[0];
        nx += (previousY - currentY) * (previousZ + currentZ);
        ny += (previousZ - currentZ) * (previousX + currentX);
        nz += (previousX - currentX) * (previousY + currentY);
    }

    const Vector3Df sum(nx, ny, nz);
    const float localNorm = norm(sum);
    if ( localNorm < EPSILON ) {
        return false;
    }
    const Vector3Df unit = inverseScaled(localNorm, sum, EPSILON_FLOAT);
    normalX = unit.x();
    normalY = unit.y();
    normalZ = unit.z();
    return true;
}

/**
Area and Jacobian J(u, v) = A + B.u + C.v of the mapping from the unit square
(quadrilaterals) or the standard triangle (0, 0), (1, 0), (0, 1). The area of a
quadrilateral is A + (B + C) / 2; B = C = 0 for parallelograms, and the
coefficients are only kept when they are not negligible. Requires the normal
*/
void
PlanarFace::computeAreaAndJacobian() {
    jacobianPresent = false;
    if ( numberOfVertices == 3 ) {
        const Vector3Df d1 = difference(getPosition(1), getPosition(0));
        const Vector3Df d2 = difference(getPosition(2), getPosition(1));
        area = 0.5F * norm(cross(d1, d2));
        return;
    }

    const Vector3Df normal = getNormal();
    const Vector3Df d1 = difference(getPosition(1), getPosition(0));
    const Vector3Df d2 = difference(getPosition(2), getPosition(1));
    const Vector3Df d3 = difference(getPosition(2), getPosition(3));
    const Vector3Df d4 = difference(getPosition(3), getPosition(0));
    const float a = dot(cross(d1, d4), normal);
    float b = dot(cross(d1, d3), normal);
    float c = dot(cross(d2, d4), normal);

    area = a + 0.5F * (b + c);
    if ( area < 0.0 ) {
        // Normal direction and vertex order are not consistent
        b = -b;
        c = -c;
        area = -area;
    }

    if ( !(java::Math::abs(b) / area < EPSILON && java::Math::abs(c) / area < EPSILON) ) {
        jacobianPresent = true;
        jacobianA = a;
        jacobianB = b;
        jacobianC = c;
    }
}

void
PlanarFace::computeMidPoint() {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
    for ( int i = 0; i < numberOfVertices; i++ ) {
        x = x + positionX[i];
        y = y + positionY[i];
        z = z + positionZ[i];
    }
    const Vector3Df mid = inverseScaled(static_cast<float>(numberOfVertices), Vector3Df(x, y, z), EPSILON_FLOAT);
    midPointX = mid.x();
    midPointY = mid.y();
    midPointZ = mid.z();
}

/**
A "width" of the plane, for coplanarity tests: the largest distance of a
vertex to the plane plus the vertex's position tolerance
*/
float
PlanarFace::computeTolerance() const {
    const Vector3Df normal = getNormal();
    float localTolerance = 0.0F;
    for ( int i = 0; i < numberOfVertices; i++ ) {
        const Vector3Df p = getPosition(i);
        const float e = java::Math::abs(dot(normal, p) + planeConstant) + positionTolerance(p, EPSILON_FLOAT);
        if ( e > localTolerance ) {
            localTolerance = e;
        }
    }
    return localTolerance;
}

/**
Index of the largest absolute component; ties go to x, then y. Used to turn
point in polygon tests into 2D problems
*/
int
PlanarFace::dominantCoordinate(const Vector3Df &vector) {
    const float ax = java::Math::abs(vector.x());
    const float ay = java::Math::abs(vector.y());
    const float az = java::Math::abs(vector.z());
    double indexValue = java::Math::max(ay, az);
    indexValue = java::Math::max(ax, static_cast<float>(indexValue));

    if ( indexValue == ax ) {
        return 0;
    }
    return indexValue == ay ? 1 : 2;
}

double
PlanarFace::clipToUnitInterval(const double x) {
    if ( x < EPSILON ) {
        return EPSILON;
    }
    return x > (1.0 - EPSILON) ? 1.0 - EPSILON : x;
}

/**
Looks for the solution of A.u^2 + B.u + C = 0 in [0, 1]; there must be exactly
one. Returns false when there is no real solution, or both or none are in the
interval (numerical inaccuracy); a best guess is returned then
*/
bool
PlanarFace::solveQuadraticUnitInterval(double A, const double B, const double C, double *x) {
    double D = B * B - 4.0 * A * C;
    double x1;
    double x2;

    if ( A < TOLERANCE && A > -TOLERANCE ) {
        // Degenerate case, solve B*x + C = 0
        x1 = -1.0;
        x2 = -C / B;
    } else {
        if ( D < -TOLERANCE * TOLERANCE ) {
            *x = -B / (2. * A);
            Logger::reportMessage("Patch", Logger::ERROR, "", java::String::format("Bi-linear->Uniform mapping has negative discriminant D = %g.\nTaking 0 as discriminant and %g as solution.", D, *x));
            return false;
        }

        D = D > TOLERANCE * TOLERANCE ? java::Math::sqrt(D) : 0.0;
        A = 1.0 / (2.0 * A);
        x1 = (-B + D) * A;
        x2 = (-B - D) * A;

        if ( x1 > -TOLERANCE && x1 < 1.0 + TOLERANCE ) {
            *x = x1;
            // Both in the interval: ambiguous, x1 taken
            return !(x2 > -TOLERANCE && x2 < 1.0 + TOLERANCE);
        }
    }

    if ( x2 > -TOLERANCE && x2 < 1.0 + TOLERANCE ) {
        *x = x2;
        return true;
    }

    // May happen due to numerical errors: choose the root closest to [0, 1]
    double d;
    if ( x1 > 1.0 ) {
        d = x1 - 1.0;
    } else {
        d = -x1;
    }
    *x = x1;
    if ( x2 > 1.0 ) {
        if ( x2 - 1.0 < d ) {
            *x = x2;
        }
    } else {
        if ( 0.0 - x2 < d ) {
            *x = x2;
        }
    }

    if ( *x < 0.0 ) {
        *x = 0.0;
    }
    if ( *x > 1.0 ) {
        *x = 1.0;
    }
    return false;
}

/**
Position i projected along the dominant axis
*/
inline void
PlanarFace::projectedPosition(const int i, double *a, double *b) const {
    switch ( dominantAxis ) {
        case 0:
            *a = positionY[i];
            *b = positionZ[i];
            break;
        case 1:
            *a = positionX[i];
            *b = positionZ[i];
            break;
        default:
            *a = positionX[i];
            *b = positionY[i];
            break;
    }
}

/**
Barycentric (u, v) of a point in the triangle. Didier Badouel, Graphics
Gems I, p390
*/
bool
PlanarFace::triangleParameters(const Vector3Df &point, Vector2Dd *uv) const {
    double u0;
    double v0;
    double pointA;
    double pointB;
    switch ( dominantAxis ) {
        case 0:
            pointA = point.y();
            pointB = point.z();
            break;
        case 1:
            pointA = point.x();
            pointB = point.z();
            break;
        default:
            pointA = point.x();
            pointB = point.y();
            break;
    }
    projectedPosition(0, &u0, &v0);
    double a1;
    double b1;
    double a2;
    double b2;
    projectedPosition(1, &a1, &b1);
    projectedPosition(2, &a2, &b2);
    const Vector2Dd p0(pointA - u0, pointB - v0);
    const Vector2Dd p1(a1 - u0, b1 - v0);
    const Vector2Dd p2(a2 - u0, b2 - v0);

    double alpha;
    double beta;
    if ( p1.x < -EPSILON || p1.x > EPSILON ) {
        beta = (p0.y * p1.x - p0.x * p1.y) / (p2.y * p1.x - p2.x * p1.y);
        if ( beta >= 0.0 && beta <= 1.0 ) {
            alpha = (p0.x - beta * p2.x) / p1.x;
        } else {
            return false;
        }
    } else {
        beta = p0.x / p2.x;
        if ( beta >= 0.0 && beta <= 1.0 ) {
            alpha = (p0.y - beta * p2.y) / p1.y;
        } else {
            return false;
        }
    }
    uv->x = alpha;
    uv->y = beta;
    return !(alpha < 0.0 || (alpha + beta) > 1.0);
}

/**
Bi-linear (u, v) of a point in the quadrilateral. Christophe Schlick and
Gilles Subrenat, "Ray Intersection of Tessellated Surfaces: Quadrangles versus
Triangles", Graphics Gems V, pages 232-241
*/
bool
PlanarFace::quadrilateralParameters(const Vector3Df &point, Vector2Dd *uv) const {
    double ax;
    double ay;
    double bx;
    double by;
    double cx;
    double cy;
    double dx;
    double dy;
    projectedPosition(0, &ax, &ay);
    projectedPosition(1, &bx, &by);
    projectedPosition(2, &cx, &cy);
    projectedPosition(3, &dx, &dy);
    const Vector2Dd A(ax, ay);
    const Vector2Dd B(bx, by);
    const Vector2Dd C(cx, cy);
    const Vector2Dd D(dx, dy);
    double pointA;
    double pointB;
    switch ( dominantAxis ) {
        case 0:
            pointA = point.y();
            pointB = point.z();
            break;
        case 1:
            pointA = point.x();
            pointB = point.z();
            break;
        default:
            pointA = point.x();
            pointB = point.y();
            break;
    }
    const Vector2Dd M(pointA, pointB);

    const Vector2Dd AB(B.x - A.x, B.y - A.y);
    const Vector2Dd BC(C.x - B.x, C.y - B.y);
    const Vector2Dd CD(D.x - C.x, D.y - C.y);
    const Vector2Dd AD(D.x - A.x, D.y - A.y);
    Vector2Dd AE(CD.x + AB.x, CD.y + AB.y);
    AE = Vector2Dd(-AE.x, -AE.y);
    const Vector2Dd AM(M.x - A.x, M.y - A.y);

    double u = -1.0;
    double v = -1.0;
    bool isInside = false;
    double a;
    double b;
    double c;

    if ( java::Math::abs(determinant(AB, CD)) < EPSILON ) {
        // AB parallel to CD
        const Vector2Dd vector(AB.x - CD.x, AB.y - CD.y);
        v = determinant(AM, vector) / determinant(AD, vector);
        if ( (v >= 0.0) && (v <= 1.0) ) {
            b = determinant(AB, AD) - determinant(AM, AE);
            c = determinant(AM, AD);
            u = java::Math::abs(b) < EPSILON ? -1 : c / b;
            isInside = ((u >= 0.0) && (u <= 1.0));
        }
    } else if ( java::Math::abs(determinant(BC, AD)) < EPSILON ) {
        // AD parallel to BC
        const Vector2Dd vector(AD.x + BC.x, AD.y + BC.y);
        u = determinant(AM, vector) / determinant(AB, vector);
        if ( (u >= 0.0) && (u <= 1.0) ) {
            b = determinant(AD, AB) - determinant(AM, AE);
            c = determinant(AM, AB);
            v = java::Math::abs(b) < EPSILON ? -1 : c / b;
            isInside = ((v >= 0.0) && (v <= 1.0));
        }
    } else {
        // General case
        a = determinant(AB, AE);
        c = -determinant(AM, AD);
        b = determinant(AB, AD) - determinant(AM, AE);
        a = -0.5 / a;
        b *= a;
        c *= (a + a);
        double sqrtDelta = b * b + c;
        if ( sqrtDelta >= 0.0 ) {
            sqrtDelta = java::Math::sqrt(sqrtDelta);
            u = b - sqrtDelta;
            if ( (u < 0.0) || (u > 1.0) ) {
                // To choose u between 0 and 1
                u = b + sqrtDelta;
            }
            if ( (u >= 0.0) && (u <= 1.0) ) {
                v = AD.x + u * AE.x;
                if ( java::Math::abs(v) < EPSILON ) {
                    v = (AM.y - u * AB.y) / (AD.y + u * AE.y);
                } else {
                    v = (AM.x - u * AB.x) / v;
                }
                isInside = ((v >= 0.0) && (v <= 1.0));
            }
        } else {
            u = v = -1.0;
        }
    }

    uv->x = clipToUnitInterval(u);
    uv->y = clipToUnitInterval(v);
    return isInside;
}

bool
PlanarFace::parameters(const Vector3Df &point, double *u, double *v) const {
    Vector2Dd uv;
    const bool inside = numberOfVertices == 3 ? triangleParameters(point, &uv) : quadrilateralParameters(point, &uv);
    *u = uv.x;
    *v = uv.y;
    return inside;
}

Vector3Df
PlanarFace::pointAt(double u, double v) const {
    if ( numberOfVertices == 3 ) {
        if ( u + v > 1.0 ) {
            u = 1.0 - u;
            v = 1.0 - v;
        }
        return pointInTriangle(getPosition(0), getPosition(1), getPosition(2), static_cast<float>(u), static_cast<float>(v));
    }
    return pointInQuadrilateral(getPosition(0), getPosition(1), getPosition(2), getPosition(3), static_cast<float>(u), static_cast<float>(v));
}

/**
Uniform coordinates are such that the area left of the line of points with
u-coordinate u is u.A and the area under the line with v-coordinate v is v.A
*/
void
PlanarFace::biLinearToUniform(double *u, double *v) const {
    const double a = jacobianA;
    const double b = jacobianB;
    const double c = jacobianC;
    *u = ((a + 0.5 * c) + 0.5 * b * (*u)) * (*u) / area;
    *v = ((a + 0.5 * b) + 0.5 * c * (*v)) * (*v) / area;
}

void
PlanarFace::uniformToBiLinear(double *u, double *v) const {
    const double a = jacobianA;
    const double b = jacobianB;
    const double c = jacobianC;
    double A = 0.5 * b / area;
    double B = (a + 0.5 * c) / area;
    double C = -(*u);
    solveQuadraticUnitInterval(A, B, C, u);

    A = 0.5 * c / area;
    B = (a + 0.5 * b) / area;
    C = -(*v);
    solveQuadraticUnitInterval(A, B, C, v);
}

Vector3Df
PlanarFace::uniformPointAt(double u, double v) const {
    if ( jacobianPresent ) {
        uniformToBiLinear(&u, &v);
    }
    return pointAt(u, v);
}

bool
PlanarFace::uniformParameters(const Vector3Df &point, double *u, double *v) const {
    const bool inside = parameters(point, u, v);
    if ( jacobianPresent ) {
        biLinearToUniform(u, v);
    }
    return inside;
}

bool
PlanarFace::isAtLeastPartlyInFront(const PlanarFace &other) const {
    const Vector3Df otherNormal = other.getNormal();
    for ( int i = 0; i < numberOfVertices; i++ ) {
        const Vector3Df p = getPosition(i);
        const double ep = dot(otherNormal, p) + other.planeConstant;
        const double localTolerance = other.tolerance + positionTolerance(p, EPSILON_FLOAT);
        if ( ep > localTolerance ) {
            return true;
        }
    }
    return false;
}

bool
PlanarFace::facing(const PlanarFace &other) const {
    return isAtLeastPartlyInFront(other) && other.isAtLeastPartlyInFront(*this);
}

float
PlanarFace::planeDistance(const Vector3Df &point) const {
    return normalX * point.x() + normalY * point.y() + normalZ * point.z() + planeConstant;
}

PlanarFace::Side
PlanarFace::sideFacing(const Vector3Df &direction, float *normalDotDirection) const {
    const float d = normalX * direction.x() + normalY * direction.y() + normalZ * direction.z();
    *normalDotDirection = d;
    if ( d > EPSILON ) {
        return Side::BACK;
    }
    if ( d < -EPSILON ) {
        return Side::FRONT;
    }
    return Side::PARALLEL;
}

float
PlanarFace::planeHitParameter(const Vector3Df &origin, const float normalDotDirection) const {
    return -(normalX * origin.x() + normalY * origin.y() + normalZ * origin.z() + planeConstant) / normalDotDirection;
}

bool
PlanarFace::rayHit(
    const Vector3Df &origin,
    const Vector3Df &direction,
    const bool acceptFront,
    const bool acceptBack,
    const float minimumDistance,
    const float maximumDistance,
    RayHit *hit) const
{
    float normalDotDirection;
    const Side side = sideFacing(direction, &normalDotDirection);
    if ( side == Side::PARALLEL
         || (side == Side::FRONT && !acceptFront)
         || (side == Side::BACK && !acceptBack) ) {
        return false;
    }
    const float distance = planeHitParameter(origin, normalDotDirection);
    if ( distance > maximumDistance || distance < minimumDistance ) {
        return false;
    }
    hit->side = side;
    hit->distance = distance;
    hit->pointX = origin.x() + distance * direction.x();
    hit->pointY = origin.y() + distance * direction.y();
    hit->pointZ = origin.z() + distance * direction.z();
    return parameters(Vector3Df(hit->pointX, hit->pointY, hit->pointZ), &hit->u, &hit->v);
}
