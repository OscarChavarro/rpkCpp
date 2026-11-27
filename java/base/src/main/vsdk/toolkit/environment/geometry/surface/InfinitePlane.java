package vsdk.toolkit.environment.geometry.surface;
import java.io.Serial;

import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.Geometry;
import vsdk.toolkit.environment.geometry.element.GeometryBounds;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.environment.geometry.element.RayBoundary;
import vsdk.toolkit.environment.geometry.element.RayCrossing;
import vsdk.toolkit.environment.geometry.element.RayCrossingKind;
import vsdk.toolkit.environment.geometry.element.RayCrossingQuery;
import vsdk.toolkit.environment.geometry.element.RayCrossingSink;
import vsdk.toolkit.environment.geometry.element.RayCrossingSupport;
import vsdk.toolkit.environment.geometry.element.RayFirstHitStatus;
import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.environment.geometry.element.RayHit;

public class InfinitePlane extends HalfSpace {
    @Serial private static final long serialVersionUID = 20060502L;

    // This is the infinite plane with canonical equation ax + bx + cx + d = 0
    private double a;
    private double b;
    private double c;
    private double d;

    public InfinitePlane(InfinitePlane other)
    {
        this.clone(other);
    }

    public InfinitePlane(double a, double b, double c, double d) {
        this.a = a;
        this.b = b;
        this.c = c;
        this.d = d;
    }

    public InfinitePlane(Vector3Dd normal, Vector3Dd pointInPlane) {
        normal = normal.normalized();
        a = normal.x();
        b = normal.y();
        c = normal.z();
        d = -normal.dotProduct(pointInPlane);
    }

    public InfinitePlane(Vector3Dd p0, Vector3Dd p1, Vector3Dd p2) {
        Vector3Dd aa;
        Vector3Dd bb;
        Vector3Dd normal;
        aa = p1.subtract(p0);
        aa = aa.normalized();
        bb = p2.subtract(p0);
        bb = bb.normalized();
        normal = aa.crossProduct(bb);
        normal = normal.normalized();
        this.a = normal.x();
        this.b = normal.y();
        this.c = normal.z();
        this.d = -normal.dotProduct(p0);
    }

    public final void clone(InfinitePlane other)
    {
        this.a = other.a;
        this.b = other.b;
        this.c = other.c;
        this.d = other.d;
    }
    @Override public Geometry copyGeometry() { return new InfinitePlane(this); }

    @Override
    public void getBounds(GeometryBounds result)
    {
        if (!Double.isFinite(a) || !Double.isFinite(b) ||
            !Double.isFinite(c) || !Double.isFinite(d) ||
            (a == 0 && b == 0 && c == 0)) result.setIndeterminate();
        else result.setUnbounded();
    }

    /** `depth` is a one-element output holder. */
    private RayFirstHitStatus evaluateRay(Ray ray, RayCrossingQuery query,
        double[] depth)
    {
        final Vector3Dd origin = ray.getOrigin();
        final Vector3Dd direction = ray.getDirection();
        if (!Double.isFinite(a) || !Double.isFinite(b) ||
            !Double.isFinite(c) || !Double.isFinite(d) ||
            (a == 0 && b == 0 && c == 0) ||
            !Double.isFinite(origin.x()) || !Double.isFinite(origin.y()) ||
            !Double.isFinite(origin.z()) || !Double.isFinite(direction.x()) ||
            !Double.isFinite(direction.y()) || !Double.isFinite(direction.z()) ||
            (direction.x() == 0 && direction.y() == 0 && direction.z() == 0) ||
            Double.isNaN(query.minT) || Double.isNaN(query.maxT) || query.minT > query.maxT ||
            !Double.isFinite(query.minimumChordSquared) || query.minimumChordSquared < 0 ||
            Double.isNaN(query.minimumExteriorApproach)) return RayFirstHitStatus.InvalidInput;
        if (query.minimumChordSquared != 0 || query.minimumExteriorApproach != Double.NEGATIVE_INFINITY)
            return RayFirstHitStatus.Unsupported;
        final Vector3Dd normal = new Vector3Dd(a,b,c);
        final double squared = normal.dotProduct(normal);
        final double numerator = -(normal.dotProduct(origin) + d);
        final double denominator = normal.dotProduct(direction);
        if (!Double.isFinite(squared) || squared == 0 ||
            !Double.isFinite(numerator) || !Double.isFinite(denominator))
            return RayFirstHitStatus.Indeterminate;
        if (denominator == 0 &&
            ((a != 0 && direction.x() != 0 && a * direction.x() == 0) ||
             (b != 0 && direction.y() != 0 && b * direction.y() == 0) ||
             (c != 0 && direction.z() != 0 && c * direction.z() == 0)))
            return RayFirstHitStatus.Indeterminate;
        final RayFirstHitStatus status = intersectionDepth(numerator, denominator, 0,
            query.minT, query.maxT, true, depth);
        if (status != RayFirstHitStatus.Hit) return status;
        final Vector3Dd point = origin.add(direction.multiply(depth[0]));
        if (!Double.isFinite(point.x()) || !Double.isFinite(point.y()) ||
            !Double.isFinite(point.z())) return RayFirstHitStatus.Indeterminate;
        return RayFirstHitStatus.Hit;
    }

    @Override
    public RayCrossingSupport emitRayCrossings(Ray ray,
        RayCrossingQuery query, RayCrossingSink sink)
    {
        double[] depth = new double[1];
        final RayFirstHitStatus status = evaluateRay(ray, query, depth);
        if (status == RayFirstHitStatus.Unsupported) return RayCrossingSupport.Unsupported;
        if (status == RayFirstHitStatus.InvalidInput) return RayCrossingSupport.InvalidInput;
        if (status == RayFirstHitStatus.Indeterminate) return RayCrossingSupport.Indeterminate;
        if (status == RayFirstHitStatus.Hit)
            sink.crossing(new RayCrossing(depth[0], this, RayCrossingKind.Unclassified));
        return RayCrossingSupport.Supported;
    }

    @Override
    public RayFirstHitStatus queryFirstHit(Ray ray, RayCrossingQuery query,
        RayHit result, RayBoundary boundary)
    {
        if (result != null) result.clear();
        if (boundary != null) boundary.assign(new RayBoundary());
        double[] depthH = new double[1];
        final RayFirstHitStatus status = evaluateRay(ray, query, depthH);
        if (status != RayFirstHitStatus.Hit) return status;
        final double depth = depthH[0];
        if (boundary != null) boundary.assign(new RayBoundary(depth, this));
        if (result != null) {
            result.setHitDistance(depth);
            if (result.shouldStoreRay()) result.setRay(ray.withT(depth));
            if (result.needsAnySurfaceData()) doExtraInformation(ray, depth, result);
        }
        return RayFirstHitStatus.Hit;
    }

    @Override
    public RayFirstHitStatus querySurfaceDetail(Ray ray, RayBoundary boundary,
        RayHit hit)
    {
        if (hit != null) hit.clear();
        if (hit == null || boundary.geometry != this || !Double.isFinite(boundary.t))
            return RayFirstHitStatus.InvalidInput;
        if (boundary.getInstanceCount() != 0) return RayFirstHitStatus.Unsupported;
        final RayFirstHitStatus status = queryFirstHit(ray, new RayCrossingQuery(boundary.t, boundary.t), null);
        if (status != RayFirstHitStatus.Hit) return status;
        hit.setHitDistance(boundary.t);
        if (hit.shouldStoreRay()) hit.setRay(ray.withT(boundary.t));
        doExtraInformation(ray, boundary.t, hit);
        if (boundary.normalReversed && hit.needsNormal() &&
            (hit.getAvailableDetailMask() & RayHit.DETAIL_NORMAL) != 0)
            hit.normal = hit.normal.multiply(-1);
        return RayFirstHitStatus.Hit;
    }

    private Vector3Dd surfaceNormal()
    {
        final Vector3Dd normal = new Vector3Dd(a,b,c);
        final double squared = normal.dotProduct(normal);
        if (squared > 0 && squared < VSDK.EPSILON) return normal.multiply(1.0 / Math.sqrt(squared));
        return normal.normalized();
    }

    /**
    Intersect a plane from its precomputed signed numerator and directional
    denominator. The caller supplies the parallel and interval policies.
    `depth` is a one-element output holder.
    */
    public static RayFirstHitStatus intersectionDepth(double numerator, double denominator,
        double parallelTolerance, double minT, double maxT, boolean checkArithmetic, double[] depth)
    {
        if (checkArithmetic && (!Double.isFinite(numerator) ||
            !Double.isFinite(denominator) || !Double.isFinite(parallelTolerance) ||
            parallelTolerance < 0 || Double.isNaN(minT) || Double.isNaN(maxT) || minT > maxT))
            return RayFirstHitStatus.InvalidInput;
        final boolean parallel = parallelTolerance == 0 ? denominator == 0 :
            denominator < parallelTolerance && denominator > -parallelTolerance;
        if (parallel) return checkArithmetic && numerator == 0 ?
            RayFirstHitStatus.Indeterminate : RayFirstHitStatus.Miss;
        depth[0] = numerator / denominator;
        if (checkArithmetic && !Double.isFinite(depth[0])) return RayFirstHitStatus.Indeterminate;
        return depth[0] >= minT && depth[0] <= maxT ? RayFirstHitStatus.Hit : RayFirstHitStatus.Miss;
    }

    /** Raw coefficients preserve the equation scale; getNormal() is normalized. */
    public Vector3Dd getCoefficients() { return new Vector3Dd(a, b, c); }
    public double getConstantCoefficient() { return d; }
    public void setCoefficients(Vector3Dd normal, double constant) {
        a = normal.x(); b = normal.y(); c = normal.z(); d = constant;
    }

    public Ray
    doIntersectionFirstHit(Ray inout_rayo) {
        double denominator = a*inout_rayo.getDirection().x() + b*inout_rayo.getDirection().y() + c*inout_rayo.getDirection().z();
        if ( Math.abs(denominator) < VSDK.EPSILON ) return null;
        double t = -(a*inout_rayo.getOrigin().x() + b*inout_rayo.getOrigin().y() + c*inout_rayo.getOrigin().z() + d)/denominator;

        if ( t < 0 ) return null;

        return inout_rayo.withT(t);
    }

    @Override
    public boolean doIntersectionFirstHit(Ray inRay, RayHit outHit)
    {
        Ray hit = doIntersectionFirstHit(inRay);
        if ( hit == null ) {
            return false;
        }
        if ( outHit != null ) {
            outHit.setRay(hit);
            doExtraInformation(hit, hit.getT(), outHit);
        }
        return true;
    }

    public Ray
    doIntersectionWithNegative(Ray inout_rayo) {
        double denominator = a*inout_rayo.getDirection().x() + b*inout_rayo.getDirection().y() + c*inout_rayo.getDirection().z();
        if ( Math.abs(denominator) < VSDK.EPSILON ) {
            Ray r = new Ray(inout_rayo.getOrigin(), inout_rayo.getDirection().multiply(-1));
            Ray hit = doIntersectionFirstHit(r);
            if ( hit != null ) {
                return inout_rayo.withT(-hit.getT());
            }
            else {
                return null;
            }
        }
        double t = -(a*inout_rayo.getOrigin().x() + b*inout_rayo.getOrigin().y() + c*inout_rayo.getOrigin().z() + d)/denominator;

        return inout_rayo.withT(t);
    }

    /**
    Por a given point `p`, calculates if it lies inside, outside or 
    on surface with respect to current plane, taking the plane as an
    infinite halfspace, not as a surface.
    @param p
    @param distanceTolerance
    @return 0 if point is on the plane surface, 1 if point is outside or
    -1 if point is inside the plane - INSIDE, OUTSIDE or LIMIT constant value.
    Note that current interpretation of the plane is done as a semispace,
    where "outside" means the direction pointed by plane's normal.
    */
    // Legacy predicates in unnormalized plane units (Java parity). The half-space
    // form classifies n.p + d <= 0; the surface form reports LIMIT on the plane
    // and OUTSIDE elsewhere (its -INSIDE equals OUTSIDE).
    public int doContainmentTestHalfSpace(Vector3Dd p,
                                          double distanceTolerance) {
        double num = a*p.x() + b*p.y() + c*p.z() + d;
        int op = LIMIT;

        if( num > distanceTolerance ) {
            op = OUTSIDE;
        }
        else if( num < -distanceTolerance ) {
            op = INSIDE;
        }
        return op;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doContainmentTest.
    @return INSIDE, OUTSIDE or LIMIT constant value
    */
    @Override
    public int doContainmentTest(Vector3Dd p, double distanceTolerance)
    {
        double num = a*p.x() + b*p.y() + c*p.z() + d;
        int op = LIMIT;

        if( num > distanceTolerance ) {
            op = OUTSIDE;
        }
        else if( num < -distanceTolerance ) {
            op = -INSIDE;
        }
        return op;    
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doExtraInformation.
    @param inT
    */
    public void
    doExtraInformation(Ray inRay, double inT, 
                                  RayHit outData) {
        if ( outData == null ) return;
        outData.beginDetailEvaluation();
        outData.point = new Vector3Dd(
            inRay.getOrigin().x() + inT*inRay.getDirection().x(),
            inRay.getOrigin().y() + inT*inRay.getDirection().y(),
            inRay.getOrigin().z() + inT*inRay.getDirection().z()
        );
        outData.normal = surfaceNormal();
        if (outData.needsPoint()) outData.markDetailAvailable(RayHit.DETAIL_POINT);
        if (outData.needsNormal()) outData.markDetailAvailable(RayHit.DETAIL_NORMAL);
    }

    /**
    TODO: Current returned values are not always valid!
    @return a new 6 valued double array containing the coordinates of a min-max
    bounding box for current geometry.
    */
    @Override
    public double[] getMinMax()
    {
        double minmax[] = new double[6];
        for ( int i = 0; i < 3; i++ ) {
            minmax[i] = -Double.MAX_VALUE;
        }
        for ( int i = 3; i < 6; i++ ) {
            minmax[i] = Double.MAX_VALUE;
        }
        return minmax;
    }

    /**
    A finite part of the plane, to present it: a square of side `size`
    divided into `tiles` x `tiles` cells, centered near a given point (i.e.
    the point of the plane viewed by a camera). The center is the projection
    of the point onto the plane, moved to the nearest corner of the cells of
    a grid fixed on the plane (with a corner at the point of the plane
    closest to the origin), so the lines of the square stay in the same
    places of the plane as the point moves. The sides follow two orthonormal
    directions (u, v) on the plane, with (u, v, normal) right handed: u is
    perpendicular to z (to x if the normal is almost along z).
    @param near point near which the square is centered
    @param size side of the square
    @param tiles cells along each side of the square, at least 1
    @return a mesh of `tiles` x `tiles` quadrilaterals, or null if the plane
    is degenerated (zero normal)
    */
    public QuadMesh exportToQuadMesh(Vector3Dd near, double size, int tiles)
    {
        double length = Math.sqrt(a*a + b*b + c*c);
        if ( !(length > 0) || tiles < 1 ) {
            return null;
        }
        Vector3Dd n = getNormal();
        Vector3Dd origin = n.multiply(-d / length);
        Vector3Dd reference = Math.abs(n.z()) < 0.9 ?
            new Vector3Dd(0, 0, 1) : new Vector3Dd(1, 0, 0);
        Vector3Dd u = reference.crossProduct(n).normalized();
        Vector3Dd v = n.crossProduct(u).normalized();
        double cell = size / tiles;
        Vector3Dd offset = projectPoint(near).subtract(origin);
        double cu = Math.rint(offset.dotProduct(u) / cell) * cell;
        double cv = Math.rint(offset.dotProduct(v) / cell) * cell;
        Vector3Dd corner = origin.add(u.multiply(cu - size/2))
            .add(v.multiply(cv - size/2));
        int vertices = tiles + 1;
        QuadMesh mesh = new QuadMesh();
        double[] positions;
        int i;
        int j;
        int index;

        mesh.initVertexPositionsArray(vertices*vertices);
        positions = mesh.getVertexPositions();
        for ( i = 0; i < vertices; i++ ) {
            for ( j = 0; j < vertices; j++ ) {
                Vector3Dd p = corner.add(u.multiply(i * cell))
                    .add(v.multiply(j * cell));
                index = i*vertices + j;
                positions[3*index] = p.x();
                positions[3*index+1] = p.y();
                positions[3*index+2] = p.z();
            }
        }
        mesh.initQuadArrays(tiles*tiles);
        index = 0;
        for ( i = 0; i < tiles; i++ ) {
            for ( j = 0; j < tiles; j++ ) {
                // Counterclockwise seen from the normal
                mesh.setQuadAt(index, i*vertices + j, (i+1)*vertices + j,
                    (i+1)*vertices + j+1, i*vertices + j+1);
                index++;
            }
        }
        return mesh;
    }

    public Vector3Dd getNormal()
    {
        Vector3Dd n = new Vector3Dd(a, b, c);
        return n.normalized();
    }

    public double getD()
    {
        return d;
    }

    public void setNormal(Vector3Dd n)
    {
        n = n.normalized();
        a = n.x();
        b = n.y();
        c = n.z();
    }

    public void setD(double d)
    {
        this.d = d;
    }

    /**
    Given a plane normal and a point in the plane, this method updates
    current plane to fit that spec.
    @param p
    @param n
    */
    public void setFromPointNormal(Vector3Dd p, Vector3Dd n)
    {
        setNormal(n);
        setD(-(n.x()*p.x() + n.y()*p.y() + n.z()*p.z()));
    }

    /**
    Given point `p`, current method returns the minimum (signed) distance
    between such a point and this plane.
    @param p
    @return a number indicating minimum signed distances to current plane
    */
    public double pointDistance(Vector3Dd p)
    {
        return a*p.x() + b*p.y() + c*p.z() + d;
    }

    /**
    Given point `p`, current method returns the point in this plane such as
    its distance is the minimum to `p`. Note this correspond to "the point's
    projection to the plane" such as the projector is at 90 deg. angle with
    respect to the plane.
    @param p
    @return a new Vector3Dd containing point projection over current plane
    */
    public Vector3Dd projectPoint(Vector3Dd p)
    {
        double distance = pointDistance(p);
        Vector3Dd n = new Vector3Dd(a, b, c);
        n = n.normalized();
        n = n.multiply(distance);
        return p.subtract(n);
    }

    /**
    Given point `p`, current method returns the mirrored point of `p` with
    respect to this plane. Note that the intersection between this plane and
    the line from `p` to its mirror is the projection of p over the plane.
    @param p
    @return a new Vector3Dd with a point mirrored with respect to input point
    around current plane
    */
    public Vector3Dd mirrorPoint(Vector3Dd p)
    {
        double distance = pointDistance(p);
        Vector3Dd n = new Vector3Dd(a, b, c);
        n = n.normalized();
        n = n.multiply(2*distance);
        return p.subtract(n);
    }

    /**
    Returns `true` if `this` plane is overlaping with `other` plane.
    Note that two planes are overlaping if they are both coplanar and
    at the same distance from the origin.
    @param other
    @param tolerance
    @return true if plane overlaps other plane, false otherwise
    */
    public boolean overlapsWith(InfinitePlane other, double tolerance)
    {
        double a1, b1, c1, d1;
        double a2, b2, c2, d2;
        double l1, l2;
        Vector3Dd n1;
        Vector3Dd n2;

        a1 = this.a;
        b1 = this.b;
        c1 = this.c;
        d1 = this.d;
        a2 = other.a;
        b2 = other.b;
        c2 = other.c;
        d2 = other.d;
        n1 = new Vector3Dd(a1, b1, c1);
        n2 = new Vector3Dd(a2, b2, c2);
        l1 = n1.length();
        l2 = n2.length();
        a1 /= l1;
        b1 /= l1;
        c1 /= l1;
        d1 /= l1;
        a2 /= l2;
        b2 /= l2;
        c2 /= l2;
        d2 /= l2;
        return Math.abs(a2 - a1) <= tolerance &&
            Math.abs(b2 - b1) <= tolerance &&
            Math.abs(c2 - c1) <= tolerance &&
            Math.abs(d2 - d1) <= tolerance;
    }

    @Override
    public String toString()
    {
        String msg = "InfinitePlane: N=<" + 
            VSDK.formatDouble(a) + ", " +
            VSDK.formatDouble(b) + ", " +
            VSDK.formatDouble(c) + ">, D=" +
            VSDK.formatDouble(d);
        return msg;
    }

    /**
     * @return the a
     */
    public double getA() {
        return a;
    }

    /**
     * @param a the a to set
     */
    public void setA(double a) {
        this.a = a;
    }

    /**
     * @return the b
     */
    public double getB() {
        return b;
    }

    /**
     * @param b the b to set
     */
    public void setB(double b) {
        this.b = b;
    }

    /**
     * @return the c
     */
    public double getC() {
        return c;
    }

    /**
     * @param c the c to set
     */
    public void setC(double c) {
        this.c = c;
    }

}
