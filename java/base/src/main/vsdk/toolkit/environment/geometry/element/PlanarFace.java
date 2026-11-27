package vsdk.toolkit.environment.geometry.element;

import java.io.Serial;

import vsdk.toolkit.common.FundamentalEntity;
import vsdk.toolkit.common.linealAlgebra.Vector2Dd;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.logging.Logger;

/**
A planar triangle or quadrilateral in double precision, as used by the
radiosity solvers: the face of a polygonal mesh with its plane, area,
parametrization and uniform (area preserving) sampling.

The face copies its three or four vertex positions when it is built and
never changes afterwards, so every operation is read only and a face can be
shared by concurrent readers.

Every quantity is computed in double precision, in source operation order and
with IEEE semantics, so results are reproducible bit for bit across ports (the
C++ PlanarFace.cpp is compiled without fast-math and without floating point
contraction). Do not rewrite it with vector helpers.

Parametrizations:
- triangle: barycentric, point = p0 + u (p1 - p0) + v (p2 - p0)
- quadrilateral: bi-linear, point = p0 + (u - uv) (p1 - p0) + uv (p2 - p0)
  + (v - uv) (p3 - p0)
The Jacobian of the quadrilateral mapping is J(u, v) = A + B u + C v; for
triangles and parallelograms it is constant and equal to the area, and the
face reports that it has no Jacobian.
*/
public class PlanarFace extends FundamentalEntity {
    @Serial
    private static final long serialVersionUID = 20261120L;

    public static final int MAXIMUM_VERTICES = 4;
    public static final double EPSILON = 1e-6;

    public enum BuildStatus {
        VALID,
        UNSUPPORTED_VERTEX_COUNT,
        DEGENERATE_NORMAL
    }

    public enum Side {
        PARALLEL,
        FRONT,
        BACK
    }

    /**
    Result of rayHit: the side the ray arrives at, the ray parameter of the
    plane hit, the hit point and its (u, v) parameters
    */
    public static final class RayHit {
        public Side side;
        public double distance;
        public double pointX;
        public double pointY;
        public double pointZ;
        public double u;
        public double v;

        public RayHit() {
        }
    }

    private static final double TOLERANCE = 1e-5;

    private final double[] positionX;
    private final double[] positionY;
    private final double[] positionZ;
    private double normalX;
    private double normalY;
    private double normalZ;
    private double midPointX;
    private double midPointY;
    private double midPointZ;
    private double planeConstant;
    private double tolerance;
    private double area;
    private double jacobianA;
    private double jacobianB;
    private double jacobianC;
    private int numberOfVertices;
    private int dominantAxis; // 0: x, 1: y, 2: z
    private boolean jacobianPresent;
    private BuildStatus status;

    // Double precision helpers, written out so that the operation order is the
    // one documented by PlanarFace and does not depend on Vector3Dd's methods

    private static Vector3Dd difference(Vector3Dd a, Vector3Dd b) {
        return new Vector3Dd(a.x() - b.x(), a.y() - b.y(), a.z() - b.z());
    }

    private static Vector3Dd cross(Vector3Dd a, Vector3Dd b) {
        return new Vector3Dd(
            a.y() * b.z() - a.z() * b.y(),
            a.z() * b.x() - a.x() * b.z(),
            a.x() * b.y() - a.y() * b.x());
    }

    private static double dot(Vector3Dd a, Vector3Dd b) {
        return a.x() * b.x() + a.y() * b.y() + a.z() * b.z();
    }

    private static double norm(Vector3Dd a) {
        return Math.sqrt(a.x() * a.x() + a.y() * a.y() + a.z() * a.z());
    }

    /**
    (1 / s) v, or v when s is within epsilon of zero
    */
    private static Vector3Dd inverseScaled(double s, Vector3Dd v, double epsilon) {
        final double factor = s < -epsilon || s > epsilon ? 1.0 / s : 1.0;
        return new Vector3Dd(factor * v.x(), factor * v.y(), factor * v.z());
    }

    private static double positionTolerance(Vector3Dd p, double epsilon) {
        return epsilon * (Math.abs(p.x()) + Math.abs(p.y()) + Math.abs(p.z()));
    }

    private static double determinant(Vector2Dd a, Vector2Dd b) {
        return a.x * b.y - a.y * b.x;
    }

    private static Vector3Dd pointInTriangle(Vector3Dd v0, Vector3Dd v1, Vector3Dd v2, double u, double v) {
        return new Vector3Dd(
            v0.x() + u * (v1.x() - v0.x()) + v * (v2.x() - v0.x()),
            v0.y() + u * (v1.y() - v0.y()) + v * (v2.y() - v0.y()),
            v0.z() + u * (v1.z() - v0.z()) + v * (v2.z() - v0.z()));
    }

    private static Vector3Dd pointInQuadrilateral(
        Vector3Dd v0,
        Vector3Dd v1,
        Vector3Dd v2,
        Vector3Dd v3,
        double u,
        double v)
    {
        final double c = u * v;
        final double b = u - c;
        final double d = v - c;
        return new Vector3Dd(
            v0.x() + b * (v1.x() - v0.x()) + c * (v2.x() - v0.x()) + d * (v3.x() - v0.x()),
            v0.y() + b * (v1.y() - v0.y()) + c * (v2.y() - v0.y()) + d * (v3.y() - v0.y()),
            v0.z() + b * (v1.z() - v0.z()) + c * (v2.z() - v0.z()) + d * (v3.z() - v0.z()));
    }

    public PlanarFace() {
        positionX = new double[MAXIMUM_VERTICES];
        positionY = new double[MAXIMUM_VERTICES];
        positionZ = new double[MAXIMUM_VERTICES];
        normalX = 0.0;
        normalY = 0.0;
        normalZ = 0.0;
        midPointX = 0.0;
        midPointY = 0.0;
        midPointZ = 0.0;
        planeConstant = 0.0;
        tolerance = 0.0;
        area = 0.0;
        jacobianA = 0.0;
        jacobianB = 0.0;
        jacobianC = 0.0;
        numberOfVertices = 0;
        dominantAxis = 0;
        jacobianPresent = false;
        status = BuildStatus.UNSUPPORTED_VERTEX_COUNT;
    }

    /**
    Builds the face of the given three or four positions, in order. The face is
    invalid (see getStatus) for any other vertex count or when the polygon is
    degenerate (its normal cannot be computed)
    @param inNumberOfVertices number of vertices (3 or 4)
    @param inPositions vertex positions, copied
    */
    public PlanarFace(int inNumberOfVertices, Vector3Dd[] inPositions) {
        this();
        if ( inNumberOfVertices != 3 && inNumberOfVertices != 4 ) {
            status = BuildStatus.UNSUPPORTED_VERTEX_COUNT;
            return;
        }
        numberOfVertices = inNumberOfVertices;
        for ( int i = 0; i < numberOfVertices; i++ ) {
            positionX[i] = inPositions[i].x();
            positionY[i] = inPositions[i].y();
            positionZ[i] = inPositions[i].z();
        }

        if ( !computeNormal() ) {
            status = BuildStatus.DEGENERATE_NORMAL;
            return;
        }
        computeAreaAndJacobian();
        computeMidPoint();
        planeConstant = -dot(getNormal(), getMidPoint());
        tolerance = computeTolerance();
        dominantAxis = dominantCoordinate(getNormal());
        status = BuildStatus.VALID;
    }

    public BuildStatus getStatus() {
        return status;
    }

    public boolean isValid() {
        return status == BuildStatus.VALID;
    }

    public int getNumberOfVertices() {
        return numberOfVertices;
    }

    public Vector3Dd getPosition(int i) {
        return new Vector3Dd(positionX[i], positionY[i], positionZ[i]);
    }

    public Vector3Dd getNormal() {
        return new Vector3Dd(normalX, normalY, normalZ);
    }

    public Vector3Dd getMidPoint() {
        return new Vector3Dd(midPointX, midPointY, midPointZ);
    }

    public double getPlaneConstant() {
        return planeConstant;
    }

    public double getTolerance() {
        return tolerance;
    }

    public double getArea() {
        return area;
    }

    public int getDominantAxis() {
        return dominantAxis;
    }

    public boolean hasJacobian() {
        return jacobianPresent;
    }

    public double getJacobianA() {
        return jacobianA;
    }

    public double getJacobianB() {
        return jacobianB;
    }

    public double getJacobianC() {
        return jacobianC;
    }

    /**
    Newell's method; false for a degenerate polygon
    */
    private boolean computeNormal() {
        double nx = 0.0;
        double ny = 0.0;
        double nz = 0.0;
        final int last = numberOfVertices - 1;
        double currentX = positionX[last] - positionX[0];
        double currentY = positionY[last] - positionY[0];
        double currentZ = positionZ[last] - positionZ[0];
        for ( int i = 0; i < numberOfVertices; i++ ) {
            final double previousX = currentX;
            final double previousY = currentY;
            final double previousZ = currentZ;
            currentX = positionX[i] - positionX[0];
            currentY = positionY[i] - positionY[0];
            currentZ = positionZ[i] - positionZ[0];
            nx += (previousY - currentY) * (previousZ + currentZ);
            ny += (previousZ - currentZ) * (previousX + currentX);
            nz += (previousX - currentX) * (previousY + currentY);
        }

        final Vector3Dd sum = new Vector3Dd(nx, ny, nz);
        final double localNorm = norm(sum);
        if ( localNorm < EPSILON ) {
            return false;
        }
        final Vector3Dd unit = inverseScaled(localNorm, sum, EPSILON);
        normalX = unit.x();
        normalY = unit.y();
        normalZ = unit.z();
        return true;
    }

    /**
    Area and Jacobian J(u, v) = A + B.u + C.v of the mapping from the unit
    square (quadrilaterals) or the standard triangle (0, 0), (1, 0), (0, 1).
    The area of a quadrilateral is A + (B + C) / 2; B = C = 0 for
    parallelograms, and the coefficients are only kept when they are not
    negligible. Requires the normal
    */
    private void computeAreaAndJacobian() {
        jacobianPresent = false;
        if ( numberOfVertices == 3 ) {
            final Vector3Dd d1 = difference(getPosition(1), getPosition(0));
            final Vector3Dd d2 = difference(getPosition(2), getPosition(1));
            area = 0.5 * norm(cross(d1, d2));
            return;
        }

        final Vector3Dd normal = getNormal();
        final Vector3Dd d1 = difference(getPosition(1), getPosition(0));
        final Vector3Dd d2 = difference(getPosition(2), getPosition(1));
        final Vector3Dd d3 = difference(getPosition(2), getPosition(3));
        final Vector3Dd d4 = difference(getPosition(3), getPosition(0));
        final double a = dot(cross(d1, d4), normal);
        double b = dot(cross(d1, d3), normal);
        double c = dot(cross(d2, d4), normal);

        area = a + 0.5 * (b + c);
        if ( area < 0.0 ) {
            // Normal direction and vertex order are not consistent
            b = -b;
            c = -c;
            area = -area;
        }

        if ( !(Math.abs(b) / area < EPSILON && Math.abs(c) / area < EPSILON) ) {
            jacobianPresent = true;
            jacobianA = a;
            jacobianB = b;
            jacobianC = c;
        }
    }

    private void computeMidPoint() {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
        for ( int i = 0; i < numberOfVertices; i++ ) {
            x = x + positionX[i];
            y = y + positionY[i];
            z = z + positionZ[i];
        }
        final Vector3Dd mid = inverseScaled((double)numberOfVertices, new Vector3Dd(x, y, z), EPSILON);
        midPointX = mid.x();
        midPointY = mid.y();
        midPointZ = mid.z();
    }

    /**
    A "width" of the plane, for coplanarity tests: the largest distance of a
    vertex to the plane plus the vertex's position tolerance
    */
    private double computeTolerance() {
        final Vector3Dd normal = getNormal();
        double localTolerance = 0.0;
        for ( int i = 0; i < numberOfVertices; i++ ) {
            final Vector3Dd p = getPosition(i);
            final double e = Math.abs(dot(normal, p) + planeConstant) + positionTolerance(p, EPSILON);
            if ( e > localTolerance ) {
                localTolerance = e;
            }
        }
        return localTolerance;
    }

    /**
    Index of the largest absolute component; ties go to x, then y. Used to
    turn point in polygon tests into 2D problems
    */
    private static int dominantCoordinate(Vector3Dd vector) {
        final double ax = Math.abs(vector.x());
        final double ay = Math.abs(vector.y());
        final double az = Math.abs(vector.z());
        double indexValue = Math.max(ay, az);
        indexValue = Math.max(ax, (double)indexValue);

        if ( indexValue == ax ) {
            return 0;
        }
        return indexValue == ay ? 1 : 2;
    }

    private static double clipToUnitInterval(double x) {
        if ( x < EPSILON ) {
            return EPSILON;
        }
        return x > (1.0 - EPSILON) ? 1.0 - EPSILON : x;
    }

    /**
    Looks for the solution of A.u^2 + B.u + C = 0 in [0, 1]; there must be
    exactly one. Returns false when there is no real solution, or both or none
    are in the interval (numerical inaccuracy); a best guess is returned then
    @param x x[0] receives the solution
    */
    private static boolean solveQuadraticUnitInterval(double A, double B, double C, double[] x) {
        double D = B * B - 4.0 * A * C;
        double x1;
        double x2;

        if ( A < TOLERANCE && A > -TOLERANCE ) {
            // Degenerate case, solve B*x + C = 0
            x1 = -1.0;
            x2 = -C / B;
        } else {
            if ( D < -TOLERANCE * TOLERANCE ) {
                x[0] = -B / (2. * A);
                Logger.reportMessage("Patch", Logger.ERROR, "", String.format("Bi-linear->Uniform mapping has negative discriminant D = %g.\nTaking 0 as discriminant and %g as solution.", D, x[0]));
                return false;
            }

            D = D > TOLERANCE * TOLERANCE ? Math.sqrt(D) : 0.0;
            A = 1.0 / (2.0 * A);
            x1 = (-B + D) * A;
            x2 = (-B - D) * A;

            if ( x1 > -TOLERANCE && x1 < 1.0 + TOLERANCE ) {
                x[0] = x1;
                // Both in the interval: ambiguous, x1 taken
                return !(x2 > -TOLERANCE && x2 < 1.0 + TOLERANCE);
            }
        }

        if ( x2 > -TOLERANCE && x2 < 1.0 + TOLERANCE ) {
            x[0] = x2;
            return true;
        }

        // May happen due to numerical errors: choose the root closest to [0, 1]
        double d;
        if ( x1 > 1.0 ) {
            d = x1 - 1.0;
        } else {
            d = -x1;
        }
        x[0] = x1;
        if ( x2 > 1.0 ) {
            if ( x2 - 1.0 < d ) {
                x[0] = x2;
            }
        } else {
            if ( 0.0 - x2 < d ) {
                x[0] = x2;
            }
        }

        if ( x[0] < 0.0 ) {
            x[0] = 0.0;
        }
        if ( x[0] > 1.0 ) {
            x[0] = 1.0;
        }
        return false;
    }

    /**
    Position i projected along the dominant axis
    @param ab ab[0] receives a, ab[1] receives b
    */
    private void projectedPosition(int i, double[] ab) {
        switch ( dominantAxis ) {
            case 0:
                ab[0] = positionY[i];
                ab[1] = positionZ[i];
                break;
            case 1:
                ab[0] = positionX[i];
                ab[1] = positionZ[i];
                break;
            default:
                ab[0] = positionX[i];
                ab[1] = positionY[i];
                break;
        }
    }

    /**
    Barycentric (u, v) of a point in the triangle. Didier Badouel, Graphics
    Gems I, p390
    */
    private boolean triangleParameters(Vector3Dd point, Vector2Dd uv) {
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
        final double[] ab = new double[2];
        projectedPosition(0, ab);
        u0 = ab[0];
        v0 = ab[1];
        double a1;
        double b1;
        double a2;
        double b2;
        projectedPosition(1, ab);
        a1 = ab[0];
        b1 = ab[1];
        projectedPosition(2, ab);
        a2 = ab[0];
        b2 = ab[1];
        final Vector2Dd p0 = new Vector2Dd(pointA - u0, pointB - v0);
        final Vector2Dd p1 = new Vector2Dd(a1 - u0, b1 - v0);
        final Vector2Dd p2 = new Vector2Dd(a2 - u0, b2 - v0);

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
        uv.x = alpha;
        uv.y = beta;
        return !(alpha < 0.0 || (alpha + beta) > 1.0);
    }

    /**
    Bi-linear (u, v) of a point in the quadrilateral. Christophe Schlick and
    Gilles Subrenat, "Ray Intersection of Tessellated Surfaces: Quadrangles
    versus Triangles", Graphics Gems V, pages 232-241
    */
    private boolean quadrilateralParameters(Vector3Dd point, Vector2Dd uv) {
        double ax;
        double ay;
        double bx;
        double by;
        double cx;
        double cy;
        double dx;
        double dy;
        final double[] ab = new double[2];
        projectedPosition(0, ab);
        ax = ab[0];
        ay = ab[1];
        projectedPosition(1, ab);
        bx = ab[0];
        by = ab[1];
        projectedPosition(2, ab);
        cx = ab[0];
        cy = ab[1];
        projectedPosition(3, ab);
        dx = ab[0];
        dy = ab[1];
        final Vector2Dd A = new Vector2Dd(ax, ay);
        final Vector2Dd B = new Vector2Dd(bx, by);
        final Vector2Dd C = new Vector2Dd(cx, cy);
        final Vector2Dd D = new Vector2Dd(dx, dy);
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
        final Vector2Dd M = new Vector2Dd(pointA, pointB);

        final Vector2Dd AB = new Vector2Dd(B.x - A.x, B.y - A.y);
        final Vector2Dd BC = new Vector2Dd(C.x - B.x, C.y - B.y);
        final Vector2Dd CD = new Vector2Dd(D.x - C.x, D.y - C.y);
        final Vector2Dd AD = new Vector2Dd(D.x - A.x, D.y - A.y);
        Vector2Dd AE = new Vector2Dd(CD.x + AB.x, CD.y + AB.y);
        AE = new Vector2Dd(-AE.x, -AE.y);
        final Vector2Dd AM = new Vector2Dd(M.x - A.x, M.y - A.y);

        double u = -1.0;
        double v = -1.0;
        boolean isInside = false;
        double a;
        double b;
        double c;

        if ( Math.abs(determinant(AB, CD)) < EPSILON ) {
            // AB parallel to CD
            final Vector2Dd vector = new Vector2Dd(AB.x - CD.x, AB.y - CD.y);
            v = determinant(AM, vector) / determinant(AD, vector);
            if ( (v >= 0.0) && (v <= 1.0) ) {
                b = determinant(AB, AD) - determinant(AM, AE);
                c = determinant(AM, AD);
                u = Math.abs(b) < EPSILON ? -1 : c / b;
                isInside = ((u >= 0.0) && (u <= 1.0));
            }
        } else if ( Math.abs(determinant(BC, AD)) < EPSILON ) {
            // AD parallel to BC
            final Vector2Dd vector = new Vector2Dd(AD.x + BC.x, AD.y + BC.y);
            u = determinant(AM, vector) / determinant(AB, vector);
            if ( (u >= 0.0) && (u <= 1.0) ) {
                b = determinant(AD, AB) - determinant(AM, AE);
                c = determinant(AM, AB);
                v = Math.abs(b) < EPSILON ? -1 : c / b;
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
                sqrtDelta = Math.sqrt(sqrtDelta);
                u = b - sqrtDelta;
                if ( (u < 0.0) || (u > 1.0) ) {
                    // To choose u between 0 and 1
                    u = b + sqrtDelta;
                }
                if ( (u >= 0.0) && (u <= 1.0) ) {
                    v = AD.x + u * AE.x;
                    if ( Math.abs(v) < EPSILON ) {
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

        uv.x = clipToUnitInterval(u);
        uv.y = clipToUnitInterval(v);
        return isInside;
    }

    /**
    (u, v) of a point on the plane of the face; returns whether the point is
    inside. For points outside, (u, v) is not meaningful
    @param point point on the plane of the face
    @param uv receives u in uv.x and v in uv.y
    @return true when the point is inside the face
    */
    public boolean parameters(Vector3Dd point, Vector2Dd uv) {
        final Vector2Dd localUv = new Vector2Dd();
        final boolean inside = numberOfVertices == 3 ? triangleParameters(point, localUv) : quadrilateralParameters(point, localUv);
        uv.x = localUv.x;
        uv.y = localUv.y;
        return inside;
    }

    /**
    Point at the given barycentric (triangle) or bi-linear (quadrilateral)
    parameters; for a triangle with u + v &gt; 1, (1 - u, 1 - v) is used
    */
    public Vector3Dd pointAt(double u, double v) {
        if ( numberOfVertices == 3 ) {
            if ( u + v > 1.0 ) {
                u = 1.0 - u;
                v = 1.0 - v;
            }
            return pointInTriangle(getPosition(0), getPosition(1), getPosition(2), (double)u, (double)v);
        }
        return pointInQuadrilateral(getPosition(0), getPosition(1), getPosition(2), getPosition(3), (double)u, (double)v);
    }

    /**
    Uniform coordinates are such that the area left of the line of points with
    u-coordinate u is u.A and the area under the line with v-coordinate v is
    v.A
    @param uv (u, v) in uv.x and uv.y, replaced by the uniform coordinates
    */
    public void biLinearToUniform(Vector2Dd uv) {
        final double a = jacobianA;
        final double b = jacobianB;
        final double c = jacobianC;
        uv.x = ((a + 0.5 * c) + 0.5 * b * (uv.x)) * (uv.x) / area;
        uv.y = ((a + 0.5 * b) + 0.5 * c * (uv.y)) * (uv.y) / area;
    }

    /**
    @param uv uniform (u, v) in uv.x and uv.y, replaced by the bi-linear ones
    */
    public void uniformToBiLinear(Vector2Dd uv) {
        final double a = jacobianA;
        final double b = jacobianB;
        final double c = jacobianC;
        final double[] x = new double[1];
        double A = 0.5 * b / area;
        double B = (a + 0.5 * c) / area;
        double C = -(uv.x);
        x[0] = uv.x;
        solveQuadraticUnitInterval(A, B, C, x);
        uv.x = x[0];

        A = 0.5 * c / area;
        B = (a + 0.5 * b) / area;
        C = -(uv.y);
        x[0] = uv.y;
        solveQuadraticUnitInterval(A, B, C, x);
        uv.y = x[0];
    }

    /**
    Area preserving mapping: point at the given uniform parameters
    */
    public Vector3Dd uniformPointAt(double u, double v) {
        final Vector2Dd uv = new Vector2Dd(u, v);
        if ( jacobianPresent ) {
            uniformToBiLinear(uv);
        }
        return pointAt(uv.x, uv.y);
    }

    /**
    Area preserving mapping: uniform parameters of a point
    @param point point on the plane of the face
    @param uv receives uniform u in uv.x and v in uv.y
    @return true when the point is inside the face
    */
    public boolean uniformParameters(Vector3Dd point, Vector2Dd uv) {
        final boolean inside = parameters(point, uv);
        if ( jacobianPresent ) {
            biLinearToUniform(uv);
        }
        return inside;
    }

    /**
    True when part of this face lies in front of the plane of the other face,
    beyond the other face's tolerance
    */
    public boolean isAtLeastPartlyInFront(PlanarFace other) {
        final Vector3Dd otherNormal = other.getNormal();
        for ( int i = 0; i < numberOfVertices; i++ ) {
            final Vector3Dd p = getPosition(i);
            final double ep = dot(otherNormal, p) + other.planeConstant;
            final double localTolerance = other.tolerance + positionTolerance(p, EPSILON);
            if ( ep > localTolerance ) {
                return true;
            }
        }
        return false;
    }

    /**
    True when each face is at least partly in front of the other
    */
    public boolean facing(PlanarFace other) {
        return isAtLeastPartlyInFront(other) && other.isAtLeastPartlyInFront(this);
    }

    /**
    Signed distance of a point to the plane, in units of the normal
    */
    public double planeDistance(Vector3Dd point) {
        return normalX * point.x() + normalY * point.y() + normalZ * point.z() + planeConstant;
    }

    /**
    Side of the face a ray with the given direction arrives at: FRONT when it
    travels against the normal, PARALLEL within EPSILON
    @param direction ray direction
    @param normalDotDirection normalDotDirection[0] receives the dot product
    of the normal with the direction
    */
    public Side sideFacing(Vector3Dd direction, double[] normalDotDirection) {
        final double d = normalX * direction.x() + normalY * direction.y() + normalZ * direction.z();
        normalDotDirection[0] = d;
        if ( d > EPSILON ) {
            return Side.BACK;
        }
        if ( d < -EPSILON ) {
            return Side.FRONT;
        }
        return Side.PARALLEL;
    }

    /**
    Ray parameter of the hit with the plane, given the dot product of the
    normal with the ray direction (from sideFacing)
    */
    public double planeHitParameter(Vector3Dd origin, double normalDotDirection) {
        return -(normalX * origin.x() + normalY * origin.y() + normalZ * origin.z() + planeConstant) / normalDotDirection;
    }

    /**
    One ray against the face, in one call: sideFacing, planeHitParameter, the
    hit point origin + distance * direction and its parameters. Returns true,
    with the hit stored, when the ray arrives at an accepted side, the distance
    lies in [minimumDistance, maximumDistance] and the point is inside the
    face; returns false otherwise (the hit is then unspecified)
    */
    public boolean rayHit(
        Vector3Dd origin,
        Vector3Dd direction,
        boolean acceptFront,
        boolean acceptBack,
        double minimumDistance,
        double maximumDistance,
        RayHit hit)
    {
        final double[] normalDotDirection = new double[1];
        final Side side = sideFacing(direction, normalDotDirection);
        if ( side == Side.PARALLEL
             || (side == Side.FRONT && !acceptFront)
             || (side == Side.BACK && !acceptBack) ) {
            return false;
        }
        final double distance = planeHitParameter(origin, normalDotDirection[0]);
        if ( distance > maximumDistance || distance < minimumDistance ) {
            return false;
        }
        hit.side = side;
        hit.distance = distance;
        hit.pointX = origin.x() + distance * direction.x();
        hit.pointY = origin.y() + distance * direction.y();
        hit.pointZ = origin.z() + distance * direction.z();
        final Vector2Dd uv = new Vector2Dd();
        final boolean inside = parameters(new Vector3Dd(hit.pointX, hit.pointY, hit.pointZ), uv);
        hit.u = uv.x;
        hit.v = uv.y;
        return inside;
    }
}
