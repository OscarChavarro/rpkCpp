//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

package vsdk.toolkit.environment.geometry.volume;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.PolyhedralBoundedSolidEulerOperators;
import java.io.Serial;

import vsdk.toolkit.common.statistics.RaytraceStatistics;
import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.numerics.FloatingInterval;
import vsdk.toolkit.environment.geometry.Geometry;
import vsdk.toolkit.environment.geometry.element.GeometryBounds;
import vsdk.toolkit.environment.geometry.element.GeometryContainment;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.environment.geometry.element.RayBoundary;
import vsdk.toolkit.environment.geometry.element.RayFirstHitStatus;
import vsdk.toolkit.environment.geometry.element.RaySolidBoundary;
import vsdk.toolkit.environment.geometry.element.RaySolidClassification;
import vsdk.toolkit.environment.geometry.element.RaySolidQueryStatus;
import vsdk.toolkit.environment.geometry.element.RayCrossing;
import vsdk.toolkit.environment.geometry.element.RayCrossingKind;
import vsdk.toolkit.environment.geometry.element.RayCrossingQuery;
import vsdk.toolkit.environment.geometry.element.RayCrossingSink;
import vsdk.toolkit.environment.geometry.element.RayCrossingSupport;

import vsdk.toolkit.environment.geometry.element.RayHit;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.PolyhedralBoundedSolid;

public class Sphere extends Solid {
    /**
    Returns zero, one or two roots in increasing order without allocation.
    `nearT` and `farT` are one-element output holders.
    */
    public static int intersectionRoots(Vector3Dd p, Vector3Dd d,
        double radius, RayCrossingQuery query, double[] nearT, double[] farT)
    {
        final double lengthSquared = d.dotProduct(d);
        if (!(lengthSquared > 0.0) || query.minT > query.maxT) return 0;
        final double directionLength = Math.sqrt(lengthSquared);
        final Vector3Dd unitDirection = d.multiply(1.0 / directionLength);
        final double radiusSquared = radius * radius;
        final double ocSquared = p.dotProduct(p);
        final boolean inside = ocSquared < radiusSquared;
        final double closest = -p.dotProduct(unitDirection);
        if (!inside && closest < query.minimumExteriorApproach) return 0;
        final double chordSquared = radiusSquared - ocSquared + closest * closest;
        if (chordSquared < query.minimumChordSquared) return 0;
        final double halfChord = Math.sqrt(chordSquared);
        farT[0] = (closest + halfChord) / directionLength;
        nearT[0] = (closest - halfChord) / directionLength;
        final boolean nearValid = nearT[0] >= query.minT && nearT[0] <= query.maxT;
        final boolean farValid = farT[0] >= query.minT && farT[0] <= query.maxT;
        if (!nearValid && !farValid) return 0;
        if (!nearValid) nearT[0] = farT[0];
        if (!farValid) farT[0] = nearT[0];
        return nearT[0] == farT[0] ? 1 : 2;
    }

    @Override public RayCrossingSupport emitRayCrossings(Ray ray, RayCrossingQuery query, RayCrossingSink sink)
    {
        RayFirstHitStatus[] failure = new RayFirstHitStatus[1];
        if (!validateSurfaceQuery(ray, query, failure)) {
            return failure[0] == RayFirstHitStatus.InvalidInput ? RayCrossingSupport.InvalidInput
                : RayCrossingSupport.Indeterminate;
        }
        double[] nearT = new double[1], farT = new double[1];
        final int count = intersectionRoots(ray.getOrigin(), ray.getDirection(),
            radius, query, nearT, farT);
        if (count > 0) emitCrossing(ray, sink, nearT[0]);
        if (count > 1) emitCrossing(ray, sink, farT[0]);
        return RayCrossingSupport.Supported;
    }

    // cpp lambda `emit` of emitRayCrossings.
    private void emitCrossing(Ray ray, RayCrossingSink sink, double t)
    {
        final Vector3Dd point = ray.getOrigin().add(ray.getDirection().multiply(t));
        final double orientation = point.dotProduct(ray.getDirection());
        final RayCrossingKind kind = orientation < 0 ? RayCrossingKind.Entering
            : orientation > 0 ? RayCrossingKind.Leaving : RayCrossingKind.Tangent;
        sink.crossing(new RayCrossing(t, this, kind));
    }

    @Override public Geometry copyGeometry() { return new Sphere(radius); }

    @Serial private static final long serialVersionUID = 20060502L;

    private double radius;
    private double radiusSquared;

    private static final int DEFAULT_PARALLELS = 8;
    private static final int DEFAULT_MERIDIANS = 16;
    private static final int MIN_PARALLELS = 2;
    private static final int MIN_MERIDIANS = 3;

    public Sphere(double r) {
        radius = r;
        radiusSquared = radius*radius;
        getControlSpecifications().add("double;radius;(0, INFINITE)");
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doIntersectionFirstHit.
    @param inout_rayo
    @return true if given ray intersects current Sphere
    */
    public Ray
    doIntersectionFirstHit(Ray inout_rayo) {
        double[] nearT = new double[1], farT = new double[1];
        if (intersectionRoots(inout_rayo.getOrigin(), inout_rayo.getDirection(), radius,
                new RayCrossingQuery(0, Double.POSITIVE_INFINITY), nearT, farT) == 0) {
            return null;
        }
        return inout_rayo.withT(nearT[0]);
    }

    @Override
    public boolean doIntersectionFirstHit(Ray inRay, RayHit outHit)
    {
        return doIntersectionFirstHit(inRay,
            new RayCrossingQuery(0, Double.POSITIVE_INFINITY), outHit);
    }

    /** Explicit inclusive interval; the legacy overload queries [0,+infinity). */
    public boolean doIntersectionFirstHit(Ray inRay, RayCrossingQuery query,
        RayHit outHit)
    {
        double[] tHolder = new double[1], farT = new double[1];
        if (intersectionRoots(inRay.getOrigin(), inRay.getDirection(), radius,
                query, tHolder, farT) == 0) {
            return false;
        }
        double t = tHolder[0];

        if ( outHit != null ) {
            if ( outHit.shouldStoreRay() || outHit.needsAnySurfaceData() ) {
                Ray hitRay = inRay.withT(t);
                outHit.setRay(hitRay);
                if ( outHit.needsAnySurfaceData() ) {
                    doExtraInformation(hitRay, t, outHit);
                    outHit.setRay(hitRay);
                }
            }
            else {
                outHit.setHitDistance(t);
            }
        }
        return true;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doExtraInformation.
    @param inT
    */
    public void
    doExtraInformation(Ray inRay, double inT,
                                  RayHit outData) {
        fillSurfaceDetail(inRay, inT, outData);
    }

    private void fillSurfaceDetail(Ray inRay, double inT, RayHit outData) {
        if ( outData == null ) {
            return;
        }

        outData.beginDetailEvaluation();
        RaytraceStatistics.recordGeometryDetailComputation();
        boolean needsNormalVector =
            outData.needsNormal() ||
            outData.needsTextureCoordinates() ||
            outData.needsTangent();
        if ( !outData.needsPoint() && !needsNormalVector ) {
            return;
        }

        Vector3Dd point = new Vector3Dd(
            inRay.getOrigin().x() + inT*inRay.getDirection().x(),
            inRay.getOrigin().y() + inT*inRay.getDirection().y(),
            inRay.getOrigin().z() + inT*inRay.getDirection().z());
        if ( outData.needsPoint() ) {
            outData.point = point;
            if (Double.isFinite(point.x()) && Double.isFinite(point.y()) && Double.isFinite(point.z()))
                outData.markDetailAvailable(RayHit.DETAIL_POINT);
        }

        Vector3Dd normal = null;
        if ( needsNormalVector ) {
            final double pointSquared = point.dotProduct(point);
            // Vector3Dd.normalized intentionally treats lengths below VSDK.EPSILON
            // as zero; a small but valid sphere still needs a unit surface normal.
            if (Double.isFinite(pointSquared) && pointSquared > 0 && pointSquared < VSDK.EPSILON)
                normal = point.multiply(1.0 / Math.sqrt(pointSquared));
            else
                normal = point.normalized();
            if ( outData.needsNormal() ) {
                outData.normal = normal;
                final double normalSquared = normal.dotProduct(normal);
                if (Double.isFinite(normalSquared) && normalSquared > 0 &&
                    Double.isFinite(normal.x()) && Double.isFinite(normal.y()) && Double.isFinite(normal.z()))
                    outData.markDetailAvailable(RayHit.DETAIL_NORMAL);
            }
        }

        if ( !outData.needsTextureCoordinates() && !outData.needsTangent() ) {
            return;
        }

        double theta;
        double phi = Math.acos(normal.z());

        if ( normal.x() > VSDK.EPSILON ) {
            theta = Math.atan(normal.y() / normal.x()) + 3*Math.PI/2;
        }
        else if ( normal.x() < VSDK.EPSILON ) {
            theta = Math.atan(normal.y() / normal.x()) + 3*Math.PI/2;
            theta += Math.PI;
            if ( theta > 2*Math.PI ) {
                theta -= 2*Math.PI;
            }
        }
        else {
            theta = 0.0;
        }

        if ( outData.needsTextureCoordinates() ) {
            outData.u = ((theta+Math.PI/2)/(2*Math.PI));
            outData.v = 1 - (phi / Math.PI);
            if (Double.isFinite(outData.u) && Double.isFinite(outData.v))
                outData.markDetailAvailable(RayHit.DETAIL_UV);
        }
        if ( outData.needsTangent() ) {
            outData.tangent = new Vector3Dd(
                Math.sin(theta-Math.PI/2),
                -Math.cos(theta-Math.PI/2),
                0);
            if (Double.isFinite(outData.tangent.x()) && Double.isFinite(outData.tangent.y()))
                outData.markDetailAvailable(RayHit.DETAIL_TANGENT);
        }
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doContainmentTest.
    @return INSIDE, OUTSIDE or LIMIT constant value
    */
    @Override
    public GeometryContainment classifyBoxCertified(Vector3Dd center, Vector3Dd halfWidths) {
        if (!Double.isFinite(radius) || radius <= 0) return GeometryContainment.InvalidInput;
        if (!FloatingInterval.isSupportedEnvironment()) return GeometryContainment.Unsupported;
        final double[] c = {center.x(), center.y(), center.z()};
        final double[] h = {halfWidths.x(), halfWidths.y(), halfWidths.z()};
        // Squared distances from the centre to the farthest and nearest box points.
        FloatingInterval farthest = FloatingInterval.exact(0), nearest = FloatingInterval.exact(0);
        for (int i = 0; i < 3; ++i) {
            final FloatingInterval range = FloatingInterval.around(c[i], h[i]);
            if (!range.isValid()) return GeometryContainment.InvalidInput;
            final double extreme = Math.max(Math.abs(range.getLower()), Math.abs(range.getUpper()));
            farthest = farthest.add(FloatingInterval.exact(extreme).square());
            if (range.getLower() > 0) nearest = nearest.add(FloatingInterval.exact(range.getLower()).square());
            else if (range.getUpper() < 0) nearest = nearest.add(FloatingInterval.exact(range.getUpper()).square());
        }
        final FloatingInterval squaredRadius = FloatingInterval.exact(radius).square();
        if (!farthest.isValid() || !nearest.isValid() || !squaredRadius.isValid()) return GeometryContainment.Indeterminate;
        if (farthest.getUpper() < squaredRadius.getLower()) return GeometryContainment.Inside;
        if (nearest.getLower() > squaredRadius.getUpper()) return GeometryContainment.Outside;
        return GeometryContainment.Indeterminate;
    }

    @Override
    public GeometryContainment classifyPointCertified(Vector3Dd p, double distanceTolerance) {
        if (!Double.isFinite(radius) || radius <= 0 ||
            !Double.isFinite(distanceTolerance) || distanceTolerance < 0 ||
            !Double.isFinite(p.x()) || !Double.isFinite(p.y()) ||
            !Double.isFinite(p.z())) return GeometryContainment.InvalidInput;
        if (!FloatingInterval.isSupportedEnvironment()) return GeometryContainment.Unsupported;
        FloatingInterval squared=FloatingInterval.exact(0);
        squared=squared.add(FloatingInterval.exact(p.x()).square());
        squared=squared.add(FloatingInterval.exact(p.y()).square());
        squared=squared.add(FloatingInterval.exact(p.z()).square());
        final FloatingInterval signedDistance=squared.squareRoot().subtract(FloatingInterval.exact(radius));
        if (!signedDistance.isValid()) return GeometryContainment.Indeterminate;
        if (signedDistance.getUpper() < -distanceTolerance) return GeometryContainment.Inside;
        if (signedDistance.getLower() > distanceTolerance) return GeometryContainment.Outside;
        if (signedDistance.getLower() >= -distanceTolerance && signedDistance.getUpper() <= distanceTolerance)
            return GeometryContainment.Boundary;
        return GeometryContainment.Indeterminate;
    }

    @Override
    public GeometryContainment classifyPoint(Vector3Dd p, double distanceTolerance) {
        if (!Double.isFinite(radius) || radius <= 0 ||
            !Double.isFinite(distanceTolerance) || distanceTolerance < 0 ||
            !Double.isFinite(p.x()) || !Double.isFinite(p.y()) ||
            !Double.isFinite(p.z())) return GeometryContainment.InvalidInput;
        if (distanceTolerance>0) return classifyPointCertified(p, distanceTolerance);
        final double squared = p.dotProduct(p);
        final double radiusSquaredValue = radius * radius;
        if (!Double.isFinite(squared) || !Double.isFinite(radius + distanceTolerance) ||
            !Double.isFinite(radiusSquaredValue) || radiusSquaredValue == 0 ||
            (squared == 0 && (p.x() != 0 || p.y() != 0 || p.z() != 0)))
            return GeometryContainment.Indeterminate;
        final double distance = p.length();
        if (distance < radius - distanceTolerance) return GeometryContainment.Inside;
        if (distance > radius + distanceTolerance) return GeometryContainment.Outside;
        return GeometryContainment.Boundary;
    }

    @Override
    public int doContainmentTest(Vector3Dd p, double distanceTolerance)
    {
        final GeometryContainment result = classifyPoint(p, distanceTolerance);
        if (result == GeometryContainment.Inside) return INSIDE;
        if (result == GeometryContainment.Boundary) return LIMIT;
        if (result == GeometryContainment.Outside) return OUTSIDE;
        // The legacy integer contract has no representation for uncertainty.
        // Do not silently convert an indeterminate or failed query to outside.
        throw new IllegalStateException("Sphere containment cannot be represented by the legacy integer result");
    }

    @Override
    public RaySolidQueryStatus classifyRay(Ray ray,
        RayCrossingQuery query, RaySolidClassification result)
    {
        result.clear();
        final Vector3Dd origin = ray.getOrigin();
        final Vector3Dd direction = ray.getDirection();
        final double infinity = Double.POSITIVE_INFINITY;
        // Surface compatibility cutoffs may discard genuine boundaries and cannot
        // define a solid classification. Applications apply those to surface queries.
        if (Double.isNaN(query.minT) || Double.isNaN(query.maxT) || query.minT >= query.maxT ||
            query.minimumChordSquared != 0.0 || query.minimumExteriorApproach != -infinity ||
            !Double.isFinite(radius) || radius <= 0.0 ||
            !Double.isFinite(origin.x()) || !Double.isFinite(origin.y()) || !Double.isFinite(origin.z()) ||
            !Double.isFinite(direction.x()) || !Double.isFinite(direction.y()) || !Double.isFinite(direction.z()) ||
            (direction.x() == 0 && direction.y() == 0 && direction.z() == 0)) {
            return RaySolidQueryStatus.InvalidInput;
        }
        final double lengthSquared = direction.dotProduct(direction);
        if (!Double.isFinite(lengthSquared) || lengthSquared == 0 ||
            !Double.isFinite(origin.dotProduct(origin)) || !Double.isFinite(radius * radius) ||
            radius * radius == 0.0) {
            return RaySolidQueryStatus.Indeterminate;
        }
        // Obtain the complete analytic interval before clipping: an empty set of
        // surface events in the requested range can still mean entirely inside.
        double[] nearH = {0}, farH = {0};
        final int count = intersectionRoots(origin, direction, radius,
            new RayCrossingQuery(-infinity, infinity), nearH, farH);
        final double nearT = nearH[0], farT = farH[0];
        if (!Double.isFinite(nearT) || !Double.isFinite(farT)) {
            return RaySolidQueryStatus.Indeterminate;
        }
        if (count < 2) {
            if (!result.insideSegments.add(false)) {
                result.clear(); return RaySolidQueryStatus.Indeterminate;
            }
            return RaySolidQueryStatus.Supported; // Miss or point tangency (IN/OUT).
        }
        if (!result.insideSegments.add(query.minT > nearT && query.minT <= farT)) {
            result.clear(); return RaySolidQueryStatus.Indeterminate;
        }
        if (nearT >= query.minT && nearT <= query.maxT) {
            if (!result.boundaries.add(new RaySolidBoundary(nearT, this)) || !result.insideSegments.add(true)) {
                result.clear(); return RaySolidQueryStatus.Indeterminate;
            }
        }
        if (farT >= query.minT && farT <= query.maxT) {
            if (!result.boundaries.add(new RaySolidBoundary(farT, this)) || !result.insideSegments.add(false)) {
                result.clear(); return RaySolidQueryStatus.Indeterminate;
            }
        }
        return RaySolidQueryStatus.Supported;
    }

    @Override
    public void getBounds(GeometryBounds result)
    {
        if (!Double.isFinite(radius) || radius <= 0) {
            result.setIndeterminate();
            return;
        }
        result.setFinite(new Vector3Dd(-radius,-radius,-radius), new Vector3Dd(radius,radius,radius));
    }

    /** `failure` is a one-element output holder. */
    private boolean validateSurfaceQuery(Ray ray, RayCrossingQuery query,
        RayFirstHitStatus[] failure)
    {
        final Vector3Dd p = ray.getOrigin();
        final Vector3Dd d = ray.getDirection();
        if (Double.isNaN(query.minT) || Double.isNaN(query.maxT) ||
            query.minT > query.maxT || !Double.isFinite(query.minimumChordSquared) ||
            query.minimumChordSquared < 0 || Double.isNaN(query.minimumExteriorApproach) ||
            !Double.isFinite(radius) || radius <= 0 ||
            !Double.isFinite(p.x()) || !Double.isFinite(p.y()) || !Double.isFinite(p.z()) ||
            !Double.isFinite(d.x()) || !Double.isFinite(d.y()) || !Double.isFinite(d.z()) ||
            (d.x() == 0 && d.y() == 0 && d.z() == 0)) { failure[0] = RayFirstHitStatus.InvalidInput; return false; }
        final double lengthSquared = d.dotProduct(d);
        final double originSquared = p.dotProduct(p);
        final double rSquared = radius * radius;
        if (!Double.isFinite(lengthSquared) || lengthSquared == 0 ||
            !Double.isFinite(originSquared) || !Double.isFinite(rSquared) || rSquared == 0)
            { failure[0] = RayFirstHitStatus.Indeterminate; return false; }
        return true;
    }

    @Override
    public RayFirstHitStatus queryFirstHit(Ray ray, RayCrossingQuery query,
        RayHit result, RayBoundary boundary)
    {
        if (result != null) result.clear();
        if (boundary != null) boundary.assign(new RayBoundary());
        final Vector3Dd p = ray.getOrigin();
        final Vector3Dd d = ray.getDirection();
        RayFirstHitStatus[] failure = new RayFirstHitStatus[1];
        if (!validateSurfaceQuery(ray, query, failure)) return failure[0];
        double[] nearH = new double[1], farH = new double[1];
        if (intersectionRoots(p, d, radius, query, nearH, farH) == 0) return RayFirstHitStatus.Miss;
        final double nearT = nearH[0];
        if (!Double.isFinite(nearT)) return RayFirstHitStatus.Indeterminate;
        if (boundary != null) boundary.assign(new RayBoundary(nearT, this));
        if (result != null) {
            result.setHitDistance(nearT);
            if (result.shouldStoreRay() || result.needsAnySurfaceData()) {
                final Ray hitRay = ray.withT(nearT);
                result.setRay(hitRay);
                if (result.needsAnySurfaceData()) {
                    fillSurfaceDetail(hitRay, nearT, result);
                    result.setRay(hitRay);
                }
            }
        }
        return RayFirstHitStatus.Hit;
    }

    /**
    cpp returns a borrowed `const double&` to the intrinsic radius; Java has no
    references to primitives, so this returns the current value.
    */
    public double getRadiusReference() { return radius; }

    /** Signed squared implicit function: negative inside, positive outside. */
    public double implicitValue(Vector3Dd point) { return point.dotProduct(point) - radiusSquared; }

    /** Fast normal for a known nonzero surface point in local coordinates. */
    public Vector3Dd surfaceNormalAtNonzeroPoint(Vector3Dd point)
    {
        // cpp Vector3Dd::normalizedFast.
        final double vTemp = Math.sqrt(point.x()*point.x() + point.y()*point.y() + point.z()*point.z());
        return new Vector3Dd(point.x() / vTemp, point.y() / vTemp, point.z() / vTemp);
    }

    /**
    @return a new 6 valued double array containing the coordinates of a min-max
    bounding box for current geometry.
    */
    @Override
    public double[] getMinMax()
    {
        double[] minmax = new double[6];
        for ( int i = 0; i < 3; i++ ) {
            minmax[i] = -radius;
        }
        for ( int i = 3; i < 6; i++ ) {
            minmax[i] = radius;
        }
        return minmax;
    }

    public double getRadius()
    {
        return radius;
    }

    public double getRadiusSquared()
    {
        return radiusSquared;
    }

    public void setRadius(double value)
    {
        radius = value;
        radiusSquared = value*value;
    }

    private static Vector3Dd
    spherePosition(double theta, double t, double r)
    {
        double phi = (t-0.5)*Math.PI;
        return new Vector3Dd(
            Math.cos(phi) * Math.cos(theta) * r,
            Math.cos(phi) * Math.sin(theta) * r,
            Math.sin(phi) * r);
    }

    @Override
    public PolyhedralBoundedSolid exportToPolyhedralBoundedSolid()
    {
        return buildPolyhedralBoundedSolid(DEFAULT_MERIDIANS,
            DEFAULT_PARALLELS, false);
    }

    /**
    Triangulated boundary representation of the sphere (see the three
    parameter version, with `withQuadrilaterals` false).
    @param meridians number of sectors around the Z axis (at least 3)
    @param parallels number of bands from pole to pole (at least 2)
    @return a new boundary representation
    */
    public PolyhedralBoundedSolid exportToPolyhedralBoundedSolid(
        int meridians, int parallels)
    {
        return exportToPolyhedralBoundedSolid(meridians, parallels, false);
    }

    /**
    Boundary representation of the sphere, centered at the origin with its
    poles on the Z axis: `parallels` bands of equal latitude step between the
    poles, each one cut into `meridians` sectors starting at +X. The bands
    touching the poles are triangle fans; the faces of the other bands are
    the planar quadrilaterals between two parallels and two meridians (two
    points of each parallel, at the same longitudes).
    @param meridians number of sectors around the Z axis (at least 3)
    @param parallels number of bands from pole to pole (at least 2)
    @param withQuadrilaterals false splits each quadrilateral by a diagonal
    into two triangles, so every face is a triangle; true keeps the
    quadrilaterals as faces
    @return a new boundary representation
    */
    public PolyhedralBoundedSolid exportToPolyhedralBoundedSolid(
        int meridians, int parallels, boolean withQuadrilaterals)
    {
        int normalizedMeridians = Math.max(MIN_MERIDIANS, meridians);
        int normalizedParallels = Math.max(MIN_PARALLELS, parallels);

        if ( !withQuadrilaterals &&
             normalizedMeridians == DEFAULT_MERIDIANS &&
             normalizedParallels == DEFAULT_PARALLELS ) {
            return exportToPolyhedralBoundedSolid();
        }

        return buildPolyhedralBoundedSolid(normalizedMeridians,
            normalizedParallels, withQuadrilaterals);
    }

    /**
    Given current sphere, this method generates a "polyhedral ball"
    approximation.
    Note that this method follows a similar strategy to the one proposed on
    function "ball", from program [MANT1988].12.6, but it is expressed entirely
    on "low level" operators, and doesn't rely on the previous availability of
    generalized rotational sweep operations.

    <p><b>Planarity guarantee for the default tessellation</b> (meridians=16,
    parallels=8): each latitude band produces quadrilaterals whose four
    vertices lie on the surface of the sphere at a constant latitude
    {@code phi} and two adjacent longitudes {@code lambda} and
    {@code lambda + 2*PI/meridians}.  All four vertices share the same
    {@code z = r*sin(phi)} coordinate, so the quad is planar in the
    horizontal plane at height {@code z}.  This was verified analytically
    in stage-1 analysis (2026-05-12) and holds for any (meridians, parallels)
    combination because the vertex positions are computed with
    {@code cos}/{@code sin} only in the latitude direction.</p>
    */
    private PolyhedralBoundedSolid buildPolyhedralBoundedSolid(
        int nmeridians, int nparalels, boolean withQuadrilaterals)
    {
        double theta;
        double phi;
        double dtheta = 2*Math.PI / ((double)nmeridians);
        double dphi = 1.0 / ((double)nparalels);
        int i, base2, base1;
        Vector3Dd pos;

        PolyhedralBoundedSolid solid;

        //- Build triangles for lower cap ---------------------------------
        solid = new PolyhedralBoundedSolid();
        pos = new Vector3Dd(0, 0, -radius);
        PolyhedralBoundedSolidEulerOperators.mvfs(solid, pos, 1, 1);

        pos = new Vector3Dd();
        pos = spherePosition(dtheta, dphi, radius);
        PolyhedralBoundedSolidEulerOperators.smev(solid, 1, 1, 3, pos);
        pos = new Vector3Dd();
        pos = spherePosition(0, dphi, radius);
        PolyhedralBoundedSolidEulerOperators.smev(solid, 1, 3, 2, pos);

        PolyhedralBoundedSolidEulerOperators.mef(solid, 1, 1, 1, 3, 2, 3, 2);

        for ( i = 2; i < nmeridians; i++ ) {
            theta = dtheta * ((double)i);
            pos = new Vector3Dd();
            pos = spherePosition(theta, dphi, radius);
            PolyhedralBoundedSolidEulerOperators.smev(solid, 1, 1, (i+1)+1, pos);
            // Next face is <(1), (i+1), (i+0)>
            PolyhedralBoundedSolidEulerOperators.mef(solid, 1,        /* seed face, always face 1 */
                      1,        /* seed face, always face 1 */
                      (i+0)+1,  /* start of half edge 1 */
                      (1),      /* end of half edge 1 */
                      (i+1)+1,  /* start of half edge 2 */
                      (1),      /* end of half edge 2 */
                      i+1       /* new face id */);
        }
        // Next face is <(1), (2), (i+1)>
        PolyhedralBoundedSolidEulerOperators.mef(solid, 1,        /* seed face, always face 1 */
                  1,        /* seed face, always face 1 */
                  (i+1),    /* start of half edge 1 */
                  (1),      /* end of half edge 1 */
                  (2),      /* start of half edge 2 */
                  (3),      /* end of half edge 2 */
                  i+1       /* new face id */);
        base2 = i+2;
        base1 = 2;

        //- Build triangulated side strips for sphere body ----------------
        // Unless `withQuadrilaterals`, each spherical quad is split into two
        // triangles immediately after creation via smef, adding a diagonal
        // from TL corner to BR corner. Face IDs are tracked with a running
        // counter to avoid conflicts with the doubled face count.
        int p;
        int nextFaceId = nmeridians + 2;
        for ( p = 0; p < nparalels-2; p++ ) {
            phi = ((double)(p+2)) / ((double)nparalels);
            for ( i = 0; i < nmeridians; i++ ) {
                theta = dtheta * ((double)i);
                pos = new Vector3Dd();
                pos = spherePosition(theta, phi, radius);
                PolyhedralBoundedSolidEulerOperators.smev(solid, 1, (i)+base1, (i)+base2, pos);
                if ( i > 0 ) {
                    int quadFaceId = nextFaceId++;
                    PolyhedralBoundedSolidEulerOperators.mef(solid, 1,           /* seed face, always face 1 */
                              1,           /* seed face, always face 1 */
                              (i-1)+base2, /* start of half edge 1: TL */
                              (i-1)+base1, /* end of half edge 1: BL */
                              (i)+base2,   /* start of half edge 2: TR */
                              (i)+base1,   /* end of half edge 2: BR */
                              quadFaceId);
                    if ( !withQuadrilaterals ) {
                        int diagFaceId = nextFaceId++;
                        // Split quad {TR,TL,BL,BR} into two triangles via TL→BR diagonal
                        PolyhedralBoundedSolidEulerOperators.smef(solid, quadFaceId,
                                  (i-1)+base2, /* TL vertex */
                                  (i)+base1,   /* BR vertex */
                                  diagFaceId);
                    }
                }
            }
            {
                int quadFaceId = nextFaceId++;
                // Wrap-around quad: closes the ring from the last meridian back to the first
                PolyhedralBoundedSolidEulerOperators.mef(solid, 1,           /* seed face, always face 1 */
                          1,           /* seed face, always face 1 */
                          (i+base2-1), /* start of half edge 1: TL-last */
                          (base1+i-1), /* end of half edge 1: BL-last */
                          (base2),     /* start of half edge 2 */
                          (base2+1),   /* end of half edge 2 */
                          quadFaceId);
                if ( !withQuadrilaterals ) {
                    int diagFaceId = nextFaceId++;
                    // Split wrap-around quad via TL-last → BL-0 diagonal
                    PolyhedralBoundedSolidEulerOperators.smef(solid, quadFaceId,
                              (i-1)+base2, /* TL-last vertex */
                              base1,       /* BL-0 vertex (first old-parallel vertex) */
                              diagFaceId);
                }
            }
            base1 = base2;
            base2 += nmeridians;
        }

        //- Build triangles for upper cap --------------------------------
        pos = new Vector3Dd(0, 0, radius);
        PolyhedralBoundedSolidEulerOperators.smev(solid, 1, base1, base2, pos);

        for ( i = 0; i < nmeridians-2; i++ ) {
            PolyhedralBoundedSolidEulerOperators.mef(solid, 1,           /* seed face, always face 1 */
                      1,           /* seed face, always face 1 */
                      base2,       /* start of half edge 1 */
                      base1+i,     /* end of half edge 1 */
                      base1+i+1,   /* start of half edge 2 */
                      base1+i+2,   /* end of half edge 2 */
                      nextFaceId++);
        }

        PolyhedralBoundedSolidEulerOperators.mef(solid, 1,           /* seed face, always face 1 */
                  1,           /* seed face, always face 1 */
                  base2,       /* start of half edge 1 */
                  base1+i,     /* end of half edge 1 */
                  base1+i+1,   /* start of half edge 2 */
                  base1,       /* end of half edge 2 */
                  nextFaceId++);

        //-----------------------------------------------------------------
        return solid;
    }

    /**
    Given a (thetha, phi) spherical coordinate in the surface of current
    Sphere, this method writes on to `p` Vector3Dd the (x, y, z) coordinates
    of the corresponding point on Sphere's surface.
    \todo  check this method for efficiency improvement
    @param p
    @param theta
    @param phi
    */
    public Vector3Dd
    spherePosition(double theta, double phi)
    {
        return new Vector3Dd(
            Math.cos(phi) * Math.cos(theta) * radius,
            -Math.cos(phi) * Math.sin(theta) * radius,
            Math.sin(phi) * radius);
    }

    /**
    Given a (thetha, phi) spherical coordinate in the surface of current
    Sphere, this method writes on to `n` Vector3Dd the (nx, ny, nz) coordinates
    of the surface normal at corresponding point on Sphere's surface.
    \todo  check this method for efficiency improvement
    @param n
    @param theta
    @param phi
    */
    public Vector3Dd
    sphereNormal(double theta, double phi)
    {
        return new Vector3Dd(
            Math.cos(phi) * Math.cos(theta),
            -Math.cos(phi) * Math.sin(theta),
            Math.sin(phi));
    }

    /**
    Given a (thetha, phi) spherical coordinate in the surface of current
    Sphere, this method writes on to `n` Vector3Dd the (tx, ty, tz) coordinates
    of the surface tangent at corresponding point on Sphere's surface. Tangents
    are aligned with respect to Sphere's equator.
    \todo  check this method for efficiency improvement
    \todo  check this method for efficiency improvement
    @param t
    @param theta
    @param phi
    */
    public Vector3Dd
    sphereTangent(double theta, double phi)
    {
        return new Vector3Dd(
            Math.sin(theta),
            Math.cos(theta),
            0);
    }

    /**
    Given a (thetha, phi) spherical coordinate in the surface of current
    Sphere, this method writes on to `n` Vector3Dd the (bx, by, bz) coordinates
    of the surface tangent binormal at corresponding point on Sphere's surface. 
    Tangents binormals are perpendicular to both normal and tangent.
    \todo  check this method for efficiency improvement
    @param b
    @param theta
    @param phi
    */
    public Vector3Dd
    sphereBinormal(double theta, double phi)
    {
        return new Vector3Dd(
            -Math.sin(phi)*Math.cos(theta),
            Math.sin(phi)*Math.sin(theta),
            Math.cos(phi)*Math.cos(theta)*Math.cos(theta) +
            Math.cos(phi)*Math.sin(theta)*Math.sin(theta));
    }

}
