//= References:                                                             =
//= [BAER2002] Baeentzen, Jakob Andreas. Aanaes, Henrik. "Generating Signed =
//=     Distance Fields From Triangle Meshes",  Technical report            =
//=     IMM-TR-2002-21, Thecnical University of Denmark, 2002.              =

package vsdk.toolkit.environment.geometry.element;
import java.io.Serial;
import vsdk.toolkit.common.FundamentalEntity;
import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.processing.Containment;

public class Triangle extends FundamentalEntity
{
    @Serial private static final long serialVersionUID = 20060502L;

    private int point0;
    private int point1;
    private int point2;

    private Vector3Dd normal;

    public Triangle() {
        normal = new Vector3Dd(0, 0, 0);
    }

    /**
    @param point0 index of the first vertex
    @param point1 index of the second vertex
    @param point2 index of the third vertex
    */
    public Triangle(int point0, int point1, int point2) {
        this.point0 = point0;
        this.point1 = point1;
        this.point2 = point2;
        normal = new Vector3Dd(0, 0, 0);
    }

    /**
    @return index of vertex 0
    */
    public int getPoint0() {
        return this.point0;
    }

    /**
    @return index of vertex 1
    */
    public int getPoint1() {
        return this.point1;
    }

    /**
    @return index of vertex 2
    */
    public int getPoint2() {
        return this.point2;
    }

    /**
    @param value index of vertex 0
    */
    public void setPoint0(int value) {
        this.point0 = value;
    }

    /**
    @param value index of vertex 1
    */
    public void setPoint1(int value) {
        this.point1 = value;
    }

    /**
    @param value index of vertex 2
    */
    public void setPoint2(int value) {
        this.point2 = value;
    }

    /**
    @return triangle normal vector
    */
    public Vector3Dd getNormal() {
        return this.normal;
    }

    /**
    @param normal
    */
    public void setNormal(Vector3Dd normal) {
        this.normal = normal;
    }

    public enum ProjectionAxis { X, Y, Z }

    public static RayFirstHitStatus queryIntersection(Ray ray, Vector3Dd v0,
        Vector3Dd v1, Vector3Dd v2, RayCrossingQuery query,
        double[] t, double[] u, double[] v)
    {
        final Vector3Dd p = ray.getOrigin();
        final Vector3Dd d = ray.getDirection();
        if (Double.isNaN(query.minT) || Double.isNaN(query.maxT) || query.minT > query.maxT ||
            !Double.isFinite(query.minimumChordSquared) || query.minimumChordSquared < 0 ||
            Double.isNaN(query.minimumExteriorApproach) ||
            !Double.isFinite(p.x()) || !Double.isFinite(p.y()) || !Double.isFinite(p.z()) ||
            !Double.isFinite(d.x()) || !Double.isFinite(d.y()) || !Double.isFinite(d.z()) ||
            (d.x() == 0 && d.y() == 0 && d.z() == 0) ||
            !Double.isFinite(v0.x()) || !Double.isFinite(v0.y()) || !Double.isFinite(v0.z()) ||
            !Double.isFinite(v1.x()) || !Double.isFinite(v1.y()) || !Double.isFinite(v1.z()) ||
            !Double.isFinite(v2.x()) || !Double.isFinite(v2.y()) || !Double.isFinite(v2.z()))
            return RayFirstHitStatus.InvalidInput;
        if (query.minimumChordSquared != 0 || query.minimumExteriorApproach != Double.NEGATIVE_INFINITY)
            return RayFirstHitStatus.Unsupported;
        final double lengthSquared = d.dotProduct(d);
        if (!Double.isFinite(lengthSquared) || lengthSquared == 0) return RayFirstHitStatus.Indeterminate;
        return intersectRayCore(ray, v0, v1, v2, query.minT, query.maxT, 0, true, true, t, u, v);
    }

    public static Intersection doIntersectionWithTriangle(
        Ray ray,
        Vector3Dd v0,
        Vector3Dd v1,
        Vector3Dd v2)
    {
        double[] t = new double[1], u = new double[1], v = new double[1];
        if ( !intersectRay(ray, v0, v1, v2, t, u, v) ) return null;

        return new Intersection(
            t[0],
            ray.getOrigin().add(ray.getDirection().multiply(t[0])),
            v1.subtract(v0).crossProduct(v2.subtract(v0)).normalized()
        );
    }

    /**
    Shared allocation-free Moller-Trumbore query; returns affine t and the
    barycentric coordinates corresponding to v1 and v2.
    */
    public static boolean intersectRay(Ray ray, Vector3Dd v0,
        Vector3Dd v1, Vector3Dd v2, double[] t, double[] u, double[] v)
    {
        return intersectRayCore(ray, v0, v1, v2, VSDK.EPSILON,
            Double.POSITIVE_INFINITY, VSDK.EPSILON, false, false, t, u, v) == RayFirstHitStatus.Hit;
    }

    /** `normal` and `distance` are one-element output holders. */
    public static boolean supportingPlane(Vector3Dd p0, Vector3Dd p1,
        Vector3Dd p2, double minimumNormalLength, Vector3Dd[] normal, double[] distance)
    {
        normal[0] = p0.subtract(p1).crossProduct(p2.subtract(p1));
        final double length = normal[0].length();
        if (!Double.isFinite(length) || length == 0 || length < minimumNormalLength) return false;
        normal[0] = normal[0].multiply(1.0 / length);
        distance[0] = -normal[0].dotProduct(p0);
        return true;
    }

    public static ProjectionAxis dominantProjection(Vector3Dd normal)
    {
        final double x = Math.abs(normal.x()), y = Math.abs(normal.y());
        final double z = Math.abs(normal.z());
        return x > y ? (x > z ? ProjectionAxis.X : ProjectionAxis.Z) :
            (y > z ? ProjectionAxis.Y : ProjectionAxis.Z);
    }

    public static boolean requiresProjectionSwap(Vector3Dd p0, Vector3Dd p1,
        Vector3Dd p2, ProjectionAxis axis)
    {
        if (axis == ProjectionAxis.X)
            return (p1.y()-p2.y())*(p1.z()-p0.z()) < (p1.z()-p2.z())*(p1.y()-p0.y());
        if (axis == ProjectionAxis.Y)
            return (p1.x()-p2.x())*(p1.z()-p0.z()) < (p1.z()-p2.z())*(p1.x()-p0.x());
        return (p1.x()-p2.x())*(p1.y()-p0.y()) < (p1.y()-p2.y())*(p1.x()-p0.x());
    }

    /** Apply the optional vertex swap before testing projected containment. */
    public static boolean containsProjected(Vector3Dd p0, Vector3Dd p1,
        Vector3Dd p2, Vector3Dd point, ProjectionAxis axis)
    {
        double a0, a1, a2, b0, b1, b2, s, t;
        if (axis == ProjectionAxis.X) {
            a0=p0.y(); a1=p1.y(); a2=p2.y(); b0=p0.z(); b1=p1.z(); b2=p2.z(); s=point.y(); t=point.z();
        } else if (axis == ProjectionAxis.Y) {
            a0=p0.x(); a1=p1.x(); a2=p2.x(); b0=p0.z(); b1=p1.z(); b2=p2.z(); s=point.x(); t=point.z();
        } else {
            a0=p0.x(); a1=p1.x(); a2=p2.x(); b0=p0.y(); b1=p1.y(); b2=p2.y(); s=point.x(); t=point.y();
        }
        if ((a1-s)*(b1-b0) < (b1-t)*(a1-a0)) return false;
        if ((a2-s)*(b2-b1) < (b2-t)*(a2-a1)) return false;
        if ((a0-s)*(b0-b2) < (b0-t)*(a0-a2)) return false;
        return true;
    }

    private static RayFirstHitStatus intersectRayCore(Ray ray, Vector3Dd v0,
        Vector3Dd v1, Vector3Dd v2, double minT, double maxT,
        double determinantTolerance, boolean includeMin, boolean checkArithmetic,
        double[] t, double[] u, double[] v)
    {
        final Vector3Dd e1 = v1.subtract(v0);
        final Vector3Dd e2 = v2.subtract(v0);
        final Vector3Dd h = ray.getDirection().crossProduct(e2);
        final double determinant = e1.dotProduct(h);
        if (checkArithmetic && (!Double.isFinite(e1.x()) || !Double.isFinite(e1.y()) ||
            !Double.isFinite(e1.z()) || !Double.isFinite(e2.x()) ||
            !Double.isFinite(e2.y()) || !Double.isFinite(e2.z()) ||
            !Double.isFinite(determinant))) return RayFirstHitStatus.Indeterminate;
        Vector3Dd geometricNormal = new Vector3Dd();
        if (checkArithmetic) {
            geometricNormal = e1.crossProduct(e2);
            final double squared = geometricNormal.dotProduct(geometricNormal);
            if (!Double.isFinite(squared)) return RayFirstHitStatus.Indeterminate;
            if (squared == 0) return geometricNormal.x() == 0 && geometricNormal.y() == 0 && geometricNormal.z() == 0 ?
                RayFirstHitStatus.Miss : RayFirstHitStatus.Indeterminate;
        }
        if (checkArithmetic && determinant == 0) {
            final Vector3Dd normal = geometricNormal;
            final Vector3Dd direction = ray.getDirection();
            final double normalDotDirection = normal.dotProduct(direction);
            if (!Double.isFinite(normalDotDirection) || normalDotDirection != 0 ||
                (normal.x() != 0 && direction.x() != 0 && normal.x()*direction.x() == 0) ||
                (normal.y() != 0 && direction.y() != 0 && normal.y()*direction.y() == 0) ||
                (normal.z() != 0 && direction.z() != 0 && normal.z()*direction.z() == 0))
                return RayFirstHitStatus.Indeterminate;
            final double side = ray.getOrigin().subtract(v0).dotProduct(normal);
            if (!Double.isFinite(side)) return RayFirstHitStatus.Indeterminate;
            return side == 0 ? RayFirstHitStatus.Indeterminate : RayFirstHitStatus.Miss;
        }
        if (Math.abs(determinant) < determinantTolerance) return RayFirstHitStatus.Miss;
        final double inverse = 1.0 / determinant;
        final Vector3Dd offset = ray.getOrigin().subtract(v0);
        u[0] = inverse * offset.dotProduct(h);
        if (checkArithmetic && !Double.isFinite(u[0])) return RayFirstHitStatus.Indeterminate;
        if (u[0] < 0.0 || u[0] > 1.0) return RayFirstHitStatus.Miss;
        final Vector3Dd q = offset.crossProduct(e1);
        v[0] = inverse * ray.getDirection().dotProduct(q);
        if (checkArithmetic && !Double.isFinite(v[0])) return RayFirstHitStatus.Indeterminate;
        if (v[0] < 0.0 || u[0] + v[0] > 1.0) return RayFirstHitStatus.Miss;
        t[0] = inverse * e2.dotProduct(q);
        if (checkArithmetic && !Double.isFinite(t[0])) return RayFirstHitStatus.Indeterminate;
        return (includeMin ? t[0] >= minT : t[0] > minT) && t[0] <= maxT ?
            RayFirstHitStatus.Hit : RayFirstHitStatus.Miss;
    }

    private static Vector3Dd closestPointOnTriangle(
        Vector3Dd p,
        Vector3Dd a,
        Vector3Dd b,
        Vector3Dd c)
    {
        Vector3Dd ab = b.subtract(a);
        Vector3Dd ac = c.subtract(a);
        Vector3Dd ap = p.subtract(a);
        double d1 = ab.dotProduct(ap);
        double d2 = ac.dotProduct(ap);
        if ( d1 <= 0 && d2 <= 0 ) return a;

        Vector3Dd bp = p.subtract(b);
        double d3 = ab.dotProduct(bp);
        double d4 = ac.dotProduct(bp);
        if ( d3 >= 0 && d4 <= d3 ) return b;

        double vc = d1*d4 - d3*d2;
        if ( vc <= 0 && d1 >= 0 && d3 <= 0 ) {
            return a.add(ab.multiply(d1/(d1-d3)));
        }

        Vector3Dd cp = p.subtract(c);
        double d5 = ab.dotProduct(cp);
        double d6 = ac.dotProduct(cp);
        if ( d6 >= 0 && d5 <= d6 ) return c;

        double vb = d5*d2 - d1*d6;
        if ( vb <= 0 && d2 >= 0 && d6 <= 0 ) {
            return a.add(ac.multiply(d2/(d2-d6)));
        }

        double va = d3*d6 - d5*d4;
        if ( va <= 0 && (d4-d3) >= 0 && (d5-d6) >= 0 ) {
            return b.add(c.subtract(b).multiply((d4-d3)/((d4-d3)+(d5-d6))));
        }

        double denom = 1.0/(va+vb+vc);
        double v = vb*denom;
        double w = vc*denom;
        return a.add(ab.multiply(v)).add(ac.multiply(w));
    }

    public static int containmentTest(
        Vector3Dd p0,
        Vector3Dd p1,
        Vector3Dd p2,
        Vector3Dd p,
        double distanceTolerance)
    {
        /*
        This method calculates containment test for triangle defined by its
        3 vertex positions. It implements a region classification based
        strategy proposed in [BAER2002]. For a given triangle and with
        respect to triangle's containing plane, a point lies in one of 7
        regions:
           - R1: inside triangle
           - R2: outside triangle, near edge 1
           - R3: outside triangle, near edge 2
           - R4: outside triangle, near edge 3
           - R5: outside triangle, near vertex 1
           - R6: outside triangle, near vertex 2
           - R7: outside triangle, near vertex 3
        */
        Vector3Dd q = closestPointOnTriangle(p, p0, p1, p2);
        if ( q.subtract(p).length() <= distanceTolerance ) {
            return Containment.LIMIT.value();
        }
        return Containment.OUTSIDE.value();
    }

    public static void minMax(
        Vector3Dd p0,
        Vector3Dd p1,
        Vector3Dd p2,
        double[] mm)
    {
        mm[0] = Math.min(p0.x(), Math.min(p1.x(), p2.x()));
        mm[1] = Math.min(p0.y(), Math.min(p1.y(), p2.y()));
        mm[2] = Math.min(p0.z(), Math.min(p1.z(), p2.z()));
        mm[3] = Math.max(p0.x(), Math.max(p1.x(), p2.x()));
        mm[4] = Math.max(p0.y(), Math.max(p1.y(), p2.y()));
        mm[5] = Math.max(p0.z(), Math.max(p1.z(), p2.z()));
    }

    /**
    Provides an object to text report convertion, optimized for human
    readability and debugging. Do not use this method for serialization
    or persistence purposes.
    @return human readable representation of current triangle
    */
    @Override
    public String toString() {

        return "f < " + point0 + ", " + point1 + ", " + point2 + " >";
    }

}
