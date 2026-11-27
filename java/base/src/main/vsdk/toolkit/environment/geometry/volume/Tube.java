//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

package vsdk.toolkit.environment.geometry.volume;

import java.io.Serial;
import java.util.ArrayList;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.linealAlgebra.Matrix4x4d;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.Geometry;
import vsdk.toolkit.environment.geometry.element.GeometryBounds;
import vsdk.toolkit.environment.geometry.element.GeometryContainment;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.environment.geometry.element.RayBoundary;
import vsdk.toolkit.environment.geometry.element.RayCrossing;
import vsdk.toolkit.environment.geometry.element.RayCrossingKind;
import vsdk.toolkit.environment.geometry.element.RayCrossingQuery;
import vsdk.toolkit.environment.geometry.element.RayCrossingSink;
import vsdk.toolkit.environment.geometry.element.RayCrossingSupport;
import vsdk.toolkit.environment.geometry.element.RayFirstHitStatus;
import vsdk.toolkit.environment.geometry.element.RayHit;
import vsdk.toolkit.environment.geometry.element.RaySolidBoundary;
import vsdk.toolkit.environment.geometry.element.RaySolidClassification;
import vsdk.toolkit.environment.geometry.element.RaySolidQueryStatus;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.PolyhedralBoundedSolid;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.PolyhedralBoundedSolidEulerConstructions;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.PolyhedralBoundedSolidEulerOperators;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.PolyhedralBoundedSolidValidationEngine;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidFace;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidHalfEdge;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidLoop;

/**
A `Tube` is a thick walled cylinder (a pipe): the solid between two coaxial
cylinders of radii `internalRadius` and `externalRadius`, with its axis along
Z, its base at z = 0 and its top at z = `height` (the same frame of a `Cone`
used as a cylinder).

Two degenerate cases are supported:
  - `internalRadius` = 0: a solid cylinder (no hole).
  - `height` = 0: a washer, a planar circular lamina with a circular hole on
    the plane z = 0. It has no interior: rays hit it (from both sides) but it
    contains no points, and its boundary representation is a lamina (two
    faces sharing their loops) of genus 1.

The interior of the tube along a ray (see `classifyRay`) is the slab
0 <= z <= height, intersected with the external cylinder and minus the
internal one: up to two intervals of the parameter of the ray (two when the
ray crosses the hole).
*/
public class Tube extends Solid {
    @Serial private static final long serialVersionUID = 20261117L;

    private static final int DEFAULT_CIRCUMFERENCE_DIVISIONS = 36;
    private static final int MIN_CIRCUMFERENCE_DIVISIONS = 3;

    /// Surfaces of the tube, used to tag the ends of the interior intervals
    private static final int SURFACE_NONE = 0;
    private static final int SURFACE_BOTTOM = 1;
    private static final int SURFACE_TOP = 2;
    private static final int SURFACE_EXTERNAL = 3;
    private static final int SURFACE_INTERNAL = 4;

    private double internalRadius;
    private double externalRadius;
    private double height;

    public Tube(double internalRadius, double externalRadius, double height)
    {
        this.internalRadius = internalRadius;
        this.externalRadius = externalRadius;
        this.height = height;
        getControlSpecifications().add("double;internalRadius;[0, INFINITE)");
        getControlSpecifications().add("double;externalRadius;(0, INFINITE)");
        getControlSpecifications().add("double;height;[0, INFINITE)");
    }

    public double getInternalRadius()
    {
        return internalRadius;
    }

    public double getExternalRadius()
    {
        return externalRadius;
    }

    public double getHeight()
    {
        return height;
    }

    public void setInternalRadius(double value)
    {
        internalRadius = value;
    }

    public void setExternalRadius(double value)
    {
        externalRadius = value;
    }

    public void setHeight(double value)
    {
        height = value;
    }

    /**
    @return true if this tube has no height, so it is a washer (a planar
    lamina with a hole)
    */
    public boolean isWasher()
    {
        return height <= 0.0;
    }

    @Override
    public Geometry copyGeometry()
    {
        return new Tube(internalRadius, externalRadius, height);
    }

    /**
    @return true if the parameters describe a tube this class can work with
    */
    private boolean hasValidParameters()
    {
        return Double.isFinite(internalRadius) && Double.isFinite(externalRadius) &&
            Double.isFinite(height) && internalRadius >= 0 &&
            externalRadius > internalRadius && height >= 0;
    }

    //= Interior of the tube along a ray =================================

    /**
    Interval [start, end] of the parameter of a line, whose ends are on the
    given surfaces of the tube (`SURFACE_NONE` for an unbounded end).
    */
    private static final class Interval
    {
        double start;
        double end;
        int startSurface;
        int endSurface;

        Interval(double start, int startSurface, double end, int endSurface)
        {
            this.start = start;
            this.end = end;
            this.startSurface = startSurface;
            this.endSurface = endSurface;
        }
    }

    /**
    @return the interval of the line `origin + t * direction` inside the
    infinite cylinder of the given radius around Z, or null if the line does
    not cross it (or only touches it)
    */
    private static Interval cylinderInterval(Vector3Dd origin, Vector3Dd direction,
        double radius, int surface)
    {
        double ox = origin.x();
        double oy = origin.y();
        double dx = direction.x();
        double dy = direction.y();
        double a = dx * dx + dy * dy;
        double c = ox * ox + oy * oy - radius * radius;

        if ( a <= 0.0 ) {
            // Parallel to the axis: everything or nothing
            if ( c < 0.0 ) {
                return new Interval(Double.NEGATIVE_INFINITY, SURFACE_NONE,
                    Double.POSITIVE_INFINITY, SURFACE_NONE);
            }
            return null;
        }
        double b = 2.0 * (ox * dx + oy * dy);
        double discriminant = b * b - 4.0 * a * c;

        if ( discriminant <= 0.0 ) {
            return null;
        }
        double root = Math.sqrt(discriminant);
        // Numerically stable roots
        double q = (b < 0.0) ? -0.5 * (b - root) : -0.5 * (b + root);
        double t0 = q / a;
        double t1 = (q != 0.0) ? c / q : -t0;

        if ( t0 > t1 ) {
            double swap = t0;
            t0 = t1;
            t1 = swap;
        }
        return new Interval(t0, surface, t1, surface);
    }

    /**
    @return the interval of the line inside the slab 0 <= z <= height, or
    null if it does not cross it
    */
    private Interval slabInterval(Vector3Dd origin, Vector3Dd direction)
    {
        double oz = origin.z();
        double dz = direction.z();

        if ( dz == 0.0 ) {
            if ( oz > 0.0 && oz < height ) {
                return new Interval(Double.NEGATIVE_INFINITY, SURFACE_NONE,
                    Double.POSITIVE_INFINITY, SURFACE_NONE);
            }
            return null;
        }
        double tBottom = -oz / dz;
        double tTop = (height - oz) / dz;

        if ( tBottom < tTop ) {
            return new Interval(tBottom, SURFACE_BOTTOM, tTop, SURFACE_TOP);
        }
        return new Interval(tTop, SURFACE_TOP, tBottom, SURFACE_BOTTOM);
    }

    /**
    Calculates the parts of the line `origin + t * direction` (for every t)
    inside the tube, in increasing order. Empty intervals are discarded, so a
    line tangent to the tube has none.
    @param out receives up to two intervals
    @return number of intervals
    */
    private int computeInteriorIntervals(Vector3Dd origin, Vector3Dd direction,
        Interval[] out)
    {
        if ( isWasher() ) {
            return 0;
        }
        Interval slab = slabInterval(origin, direction);
        if ( slab == null ) {
            return 0;
        }
        Interval external = cylinderInterval(origin, direction, externalRadius,
            SURFACE_EXTERNAL);
        if ( external == null ) {
            return 0;
        }

        //- Slab intersected with the external cylinder --------------------
        Interval body = new Interval(slab.start, slab.startSurface, slab.end,
            slab.endSurface);
        if ( external.start > body.start ) {
            body.start = external.start;
            body.startSurface = external.startSurface;
        }
        if ( external.end < body.end ) {
            body.end = external.end;
            body.endSurface = external.endSurface;
        }
        if ( !(body.start < body.end) ) {
            return 0;
        }

        //- Minus the internal cylinder ------------------------------------
        Interval hole = null;
        if ( internalRadius > 0.0 ) {
            hole = cylinderInterval(origin, direction, internalRadius,
                SURFACE_INTERNAL);
        }
        if ( hole == null || hole.end <= body.start || hole.start >= body.end ) {
            out[0] = body;
            return 1;
        }

        int n = 0;
        if ( hole.start > body.start ) {
            out[n++] = new Interval(body.start, body.startSurface, hole.start,
                SURFACE_INTERNAL);
        }
        if ( hole.end < body.end ) {
            out[n++] = new Interval(hole.end, SURFACE_INTERNAL, body.end,
                body.endSurface);
        }
        return n;
    }

    /**
    @return the parameter where the line crosses the washer (the plane z = 0
    between both radii), or NaN if it does not
    */
    private double washerHit(Vector3Dd origin, Vector3Dd direction)
    {
        double dz = direction.z();

        if ( dz == 0.0 ) {
            return Double.NaN;
        }
        double t = -origin.z() / dz;
        double px = origin.x() + direction.x() * t;
        double py = origin.y() + direction.y() * t;
        double r2 = px * px + py * py;

        if ( r2 > externalRadius * externalRadius ||
             r2 < internalRadius * internalRadius ) {
            return Double.NaN;
        }
        return t;
    }

    /**
    First point of the surface of the tube hit by a ray, after `minT`.
    @param surface one-element output holder receiving the surface hit
    @return the parameter of the hit, or NaN if there is none
    */
    private double firstSurfaceHit(Ray ray, double minT, int[] surface)
    {
        Vector3Dd origin = ray.getOrigin();
        Vector3Dd direction = ray.getDirection();

        if ( isWasher() ) {
            double t = washerHit(origin, direction);
            if ( !Double.isNaN(t) && t > minT ) {
                surface[0] = direction.z() < 0 ? SURFACE_TOP : SURFACE_BOTTOM;
                return t;
            }
            return Double.NaN;
        }

        Interval[] intervals = new Interval[2];
        int n = computeInteriorIntervals(origin, direction, intervals);
        for ( int i = 0; i < n; i++ ) {
            if ( intervals[i].start > minT ) {
                surface[0] = intervals[i].startSurface;
                return intervals[i].start;
            }
            if ( intervals[i].end > minT ) {
                surface[0] = intervals[i].endSurface;
                return intervals[i].end;
            }
        }
        return Double.NaN;
    }

    //= Classic ray intersection interface ===============================

    /**
    Check the general interface contract in superclass method
    Geometry.doIntersectionFirstHit.
    @param inOutRay
    @return the ray with the distance to the hit, or null if the ray does not
    hit current Tube
    */
    public Ray doIntersectionFirstHit(Ray inOutRay)
    {
        RayHit hit = new RayHit();
        if ( doIntersectionFirstHit(inOutRay, hit) ) {
            return hit.getRay();
        }
        return null;
    }

    @Override
    public boolean doIntersectionFirstHit(Ray inOutRay, RayHit outHit)
    {
        if ( !hasValidParameters() ) {
            return false;
        }
        int[] surface = {SURFACE_NONE};
        double t = firstSurfaceHit(inOutRay, VSDK.EPSILON, surface);

        if ( Double.isNaN(t) ) {
            return false;
        }
        if ( outHit != null ) {
            if ( outHit.shouldStoreRay() || outHit.needsAnySurfaceData() ) {
                outHit.setRay(inOutRay.withT(t));
            }
            else {
                outHit.setHitDistance(t);
            }
            if ( outHit.needsAnySurfaceData() ) {
                fillSurfaceData(inOutRay, t, surface[0], outHit);
            }
        }
        return true;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doExtraInformation.
    @param inRay
    @param inT
    @param outData
    */
    @Override
    public void doExtraInformation(Ray inRay, double inT, RayHit outData)
    {
        if ( outData == null ) {
            return;
        }
        Vector3Dd p = inRay.getOrigin().add(inRay.getDirection().multiply(inT));
        fillSurfaceData(inRay, inT, surfaceAt(p, inRay.getDirection()), outData);
    }

    /**
    @return the surface of the tube nearest to a point of its boundary
    */
    private int surfaceAt(Vector3Dd p, Vector3Dd rayDirection)
    {
        if ( isWasher() ) {
            return rayDirection.z() < 0 ? SURFACE_TOP : SURFACE_BOTTOM;
        }
        double rho = Math.sqrt(p.x() * p.x() + p.y() * p.y());
        double dBottom = Math.abs(p.z());
        double dTop = Math.abs(p.z() - height);
        double dExternal = Math.abs(rho - externalRadius);
        double dInternal = internalRadius > 0 ?
            Math.abs(rho - internalRadius) : Double.POSITIVE_INFINITY;
        int surface = SURFACE_BOTTOM;
        double best = dBottom;

        if ( dTop < best ) {
            best = dTop;
            surface = SURFACE_TOP;
        }
        if ( dExternal < best ) {
            best = dExternal;
            surface = SURFACE_EXTERNAL;
        }
        if ( dInternal < best ) {
            surface = SURFACE_INTERNAL;
        }
        return surface;
    }

    private void fillSurfaceData(Ray ray, double t, int surface, RayHit outData)
    {
        outData.beginDetailEvaluation();
        Vector3Dd p = ray.getOrigin().add(ray.getDirection().multiply(t));
        double rho = Math.sqrt(p.x() * p.x() + p.y() * p.y());
        double angle = Math.atan2(p.y(), p.x());
        if ( angle < 0 ) {
            angle += 2 * Math.PI;
        }
        double c = rho > 0 ? p.x() / rho : 1.0;
        double s = rho > 0 ? p.y() / rho : 0.0;
        Vector3Dd normal;

        switch ( surface ) {
          case SURFACE_TOP -> normal = new Vector3Dd(0, 0, 1);
          case SURFACE_BOTTOM -> normal = new Vector3Dd(0, 0, -1);
          case SURFACE_INTERNAL -> normal = new Vector3Dd(-c, -s, 0);
          default -> normal = new Vector3Dd(c, s, 0);
        }

        if ( outData.needsPoint() ) {
            outData.point = p;
            outData.markDetailAvailable(RayHit.DETAIL_POINT);
        }
        if ( outData.needsNormal() ) {
            outData.normal = normal;
            outData.markDetailAvailable(RayHit.DETAIL_NORMAL);
        }
        if ( outData.needsTangent() ) {
            outData.tangent = new Vector3Dd(-s, c, 0);
            outData.markDetailAvailable(RayHit.DETAIL_TANGENT);
        }
        if ( outData.needsTextureCoordinates() ) {
            outData.u = angle / (2 * Math.PI);
            if ( surface == SURFACE_TOP || surface == SURFACE_BOTTOM ) {
                outData.v = (rho - internalRadius) / (externalRadius - internalRadius);
            }
            else {
                outData.v = height > 0 ? p.z() / height : 0;
            }
            outData.markDetailAvailable(RayHit.DETAIL_UV);
        }
    }

    //= Ray crossing and classification interface ========================

    /**
    @return null if the query and the ray can be processed, or the status
    explaining why they can not
    */
    private RayFirstHitStatus validateQuery(Ray ray, RayCrossingQuery query)
    {
        if ( Double.isNaN(query.minT) || Double.isNaN(query.maxT) ||
             query.minT > query.maxT ||
             !Double.isFinite(query.minimumChordSquared) ||
             query.minimumChordSquared < 0 ||
             Double.isNaN(query.minimumExteriorApproach) ) {
            return RayFirstHitStatus.InvalidInput;
        }
        if ( query.minimumChordSquared != 0 ||
             query.minimumExteriorApproach != Double.NEGATIVE_INFINITY ) {
            return RayFirstHitStatus.Unsupported;
        }
        Vector3Dd o = ray.getOrigin();
        Vector3Dd d = ray.getDirection();
        if ( !Double.isFinite(o.x()) || !Double.isFinite(o.y()) ||
             !Double.isFinite(o.z()) || !Double.isFinite(d.x()) ||
             !Double.isFinite(d.y()) || !Double.isFinite(d.z()) ||
             (d.x() == 0 && d.y() == 0 && d.z() == 0) ) {
            return RayFirstHitStatus.InvalidInput;
        }
        if ( !hasValidParameters() ) {
            return RayFirstHitStatus.InvalidInput;
        }
        return null;
    }

    @Override
    public RayCrossingSupport emitRayCrossings(Ray ray, RayCrossingQuery query,
        RayCrossingSink sink)
    {
        RayFirstHitStatus invalid = validateQuery(ray, query);
        if ( invalid == RayFirstHitStatus.Unsupported ) {
            return RayCrossingSupport.Unsupported;
        }
        if ( invalid != null ) {
            return RayCrossingSupport.InvalidInput;
        }

        if ( isWasher() ) {
            double t = washerHit(ray.getOrigin(), ray.getDirection());
            if ( !Double.isNaN(t) && t >= query.minT && t <= query.maxT ) {
                sink.crossing(new RayCrossing(t, this, RayCrossingKind.Tangent));
            }
            return RayCrossingSupport.Supported;
        }

        Interval[] intervals = new Interval[2];
        int n = computeInteriorIntervals(ray.getOrigin(), ray.getDirection(),
            intervals);
        for ( int i = 0; i < n; i++ ) {
            if ( !Double.isFinite(intervals[i].start) ||
                 !Double.isFinite(intervals[i].end) ) {
                return RayCrossingSupport.Indeterminate;
            }
        }
        for ( int i = 0; i < n; i++ ) {
            double entry = intervals[i].start;
            double exit = intervals[i].end;
            if ( entry >= query.minT && entry <= query.maxT ) {
                sink.crossing(new RayCrossing(entry, this, RayCrossingKind.Entering));
            }
            if ( exit >= query.minT && exit <= query.maxT ) {
                sink.crossing(new RayCrossing(exit, this, RayCrossingKind.Leaving));
            }
        }
        return RayCrossingSupport.Supported;
    }

    @Override
    public RayFirstHitStatus queryFirstHit(Ray ray, RayCrossingQuery query,
        RayHit result, RayBoundary boundary)
    {
        if ( result != null ) {
            result.clear();
        }
        if ( boundary != null ) {
            boundary.assign(new RayBoundary());
        }
        RayFirstHitStatus invalid = validateQuery(ray, query);
        if ( invalid != null ) {
            return invalid;
        }

        // The first boundary at or after minT: the surface hit is searched
        // strictly after a parameter slightly smaller than minT
        int[] surface = {SURFACE_NONE};
        double t = firstSurfaceHit(ray, Math.nextDown(query.minT), surface);

        if ( Double.isNaN(t) || t > query.maxT ) {
            return RayFirstHitStatus.Miss;
        }
        if ( boundary != null ) {
            boundary.assign(new RayBoundary(t, this));
        }
        if ( result != null ) {
            result.setHitDistance(t);
            if ( result.shouldStoreRay() ) {
                result.setRay(ray.withT(t));
            }
            if ( result.needsAnySurfaceData() ) {
                fillSurfaceData(ray, t, surface[0], result);
            }
        }
        return RayFirstHitStatus.Hit;
    }

    @Override
    public RayFirstHitStatus querySurfaceDetail(Ray ray, RayBoundary boundary,
        RayHit hit)
    {
        if ( hit != null ) {
            hit.clear();
        }
        if ( hit == null || boundary.geometry != this ||
             !Double.isFinite(boundary.t) ) {
            return RayFirstHitStatus.InvalidInput;
        }
        if ( boundary.getInstanceCount() != 0 ) {
            return RayFirstHitStatus.Unsupported;
        }
        RayFirstHitStatus status = queryFirstHit(ray,
            new RayCrossingQuery(boundary.t, boundary.t), null);
        if ( status != RayFirstHitStatus.Hit ) {
            return status;
        }
        hit.setHitDistance(boundary.t);
        if ( hit.shouldStoreRay() ) {
            hit.setRay(ray.withT(boundary.t));
        }
        doExtraInformation(ray, boundary.t, hit);
        if ( boundary.normalReversed && hit.needsNormal() &&
             (hit.getAvailableDetailMask() & RayHit.DETAIL_NORMAL) != 0 ) {
            hit.normal = hit.normal.multiply(-1);
        }
        return RayFirstHitStatus.Hit;
    }

    /**
    Classifies the ray against the interior of the tube, following the
    contract of `Geometry.classifyRay`: the first element of
    `result.insideSegments` tells if the ray starts (at `query.minT`) inside
    the tube, and each boundary toggles it. A washer has no interior.
    */
    @Override
    public RaySolidQueryStatus classifyRay(Ray ray, RayCrossingQuery query,
        RaySolidClassification result)
    {
        result.clear();
        if ( query.minT >= query.maxT ) {
            return RaySolidQueryStatus.InvalidInput;
        }
        RayFirstHitStatus invalid = validateQuery(ray, query);
        if ( invalid == RayFirstHitStatus.Unsupported ) {
            return RaySolidQueryStatus.Unsupported;
        }
        if ( invalid != null ) {
            return RaySolidQueryStatus.InvalidInput;
        }

        Interval[] intervals = new Interval[2];
        int n = computeInteriorIntervals(ray.getOrigin(), ray.getDirection(),
            intervals);
        boolean startsInside = false;

        for ( int i = 0; i < n; i++ ) {
            if ( query.minT > intervals[i].start && query.minT <= intervals[i].end ) {
                startsInside = true;
            }
        }
        result.insideSegments.add(startsInside);
        for ( int i = 0; i < n; i++ ) {
            double entry = intervals[i].start;
            double exit = intervals[i].end;
            if ( entry >= query.minT && entry <= query.maxT ) {
                result.boundaries.add(new RaySolidBoundary(entry, this));
                result.insideSegments.add(true);
            }
            if ( exit >= query.minT && exit <= query.maxT ) {
                result.boundaries.add(new RaySolidBoundary(exit, this));
                result.insideSegments.add(false);
            }
        }
        return RaySolidQueryStatus.Supported;
    }

    //= Containment ======================================================

    /**
    Check the general interface contract in superclass method
    Geometry.doContainmentTest. Distances are measured perpendicular to each
    surface (caps, external and internal walls), so the tolerance is the same
    for all of them. Every point of a washer within the tolerance of it is
    `LIMIT`, as it has no interior.
    @param p point to classify, in the space of the tube
    @param distanceTolerance distance to the surface of the tube under which
    the point is classified as `LIMIT`
    @return INSIDE, OUTSIDE or LIMIT constant value
    */
    @Override
    public int doContainmentTest(Vector3Dd p, double distanceTolerance)
    {
        if ( !hasValidParameters() ) {
            return OUTSIDE;
        }
        double z = p.z();
        double rho = Math.sqrt(p.x() * p.x() + p.y() * p.y());

        if ( z < -distanceTolerance || z > height + distanceTolerance ||
             rho > externalRadius + distanceTolerance ||
             rho < internalRadius - distanceTolerance ) {
            return OUTSIDE;
        }
        boolean insideHole = internalRadius > 0 && rho < internalRadius;
        if ( insideHole && rho < internalRadius - distanceTolerance ) {
            return OUTSIDE;
        }
        if ( z > distanceTolerance && z < height - distanceTolerance &&
             rho < externalRadius - distanceTolerance &&
             (internalRadius <= 0 || rho > internalRadius + distanceTolerance) ) {
            return INSIDE;
        }
        return LIMIT;
    }

    @Override
    public GeometryContainment classifyPoint(Vector3Dd p, double distanceTolerance)
    {
        if ( !Double.isFinite(distanceTolerance) || distanceTolerance < 0 ||
             !Double.isFinite(p.x()) || !Double.isFinite(p.y()) ||
             !Double.isFinite(p.z()) ) {
            return GeometryContainment.InvalidInput;
        }
        if ( !hasValidParameters() ) {
            return GeometryContainment.InvalidInput;
        }
        int c = doContainmentTest(p, distanceTolerance);
        if ( c == INSIDE ) {
            return GeometryContainment.Inside;
        }
        if ( c == LIMIT ) {
            return GeometryContainment.Boundary;
        }
        return GeometryContainment.Outside;
    }

    //= Bounds ===========================================================

    /** Finite bounds: |x|,|y| <= externalRadius and 0 <= z <= height. */
    @Override
    public void getBounds(GeometryBounds result)
    {
        if ( !hasValidParameters() ) {
            result.setIndeterminate();
            return;
        }
        result.setFinite(new Vector3Dd(-externalRadius, -externalRadius, 0),
            new Vector3Dd(externalRadius, externalRadius, height));
    }

    @Override
    public double[] getMinMax()
    {
        double[] minmax = new double[6];

        minmax[0] = -externalRadius;
        minmax[1] = -externalRadius;
        minmax[2] = 0;
        minmax[3] = externalRadius;
        minmax[4] = externalRadius;
        minmax[5] = height;
        return minmax;
    }

    //= Boundary representation ==========================================

    @Override
    public PolyhedralBoundedSolid exportToPolyhedralBoundedSolid()
    {
        return buildPolyhedralBoundedSolid(DEFAULT_CIRCUMFERENCE_DIVISIONS);
    }

    /**
    @param circumferenceDivisions number of sides of the polygons
    approximating the circles (at least 3)
    @return the boundary representation of the tube
    */
    public PolyhedralBoundedSolid exportToPolyhedralBoundedSolid(
        int circumferenceDivisions)
    {
        return buildPolyhedralBoundedSolid(
            Math.max(MIN_CIRCUMFERENCE_DIVISIONS, circumferenceDivisions));
    }

    /**
    Builds a planar circular lamina with a circular hole (a washer) on the
    plane z = `h`, using only Euler operators, following [MANT1988] 12.2
    (circular lamina, program 12.2) and the creation of a hole as an inner
    ring [MANT1988] 9.2.3 / 9.2.4:
      1. `createCircularLamina`: the external circle, as faces 1 and 2
         (`mvfs`, `smev` arc, `smef`).
      2. `smev` + `kemr`: a vertex of the internal circle is created hanging
         from the external one and the bridge edge is killed, so the vertex
         is left alone as an inner ring of face 1.
      3. `smev` arc: the rest of the internal circle, from that vertex.
      4. `smef`: the internal circle is closed, splitting from face 1 a new
         face (the disk of the hole).
      5. `kfmrh`: the disk is merged into face 2 as an inner ring, so both
         faces of the lamina have the hole.
    If `ri` is 0, the lamina has no hole (steps 2 to 5 are skipped).
    @param ri internal radius, or 0 for a disk
    @param re external radius
    @param h height of the plane of the lamina
    @param n number of sides of the circles (at least 3)
    @return the lamina; face 1 is the one facing +Z (it is the one to sweep)
    */
    public static PolyhedralBoundedSolid createWasherLamina(double ri,
        double re, double h, int n)
    {
        return createWasherLamina(ri, re, h, n, false);
    }

    /**
    Builds the washer lamina of `createWasherLamina(ri, re, h, n)`.

    With `withQuadrilaterals`, each face of the lamina is further split so
    that the lamina is made of small planar polygons instead of two faces
    with a hole, as needed by programs that use the faces as patches:
      - with a hole: the two loops of each face are joined by an edge
        between the external vertex 1 and the internal vertex at its same
        angle (`lmekr`), and the face is split (`smef`) by an edge between
        every other external vertex and the internal vertex at its angle.
        Each face of the lamina becomes `n` quadrilaterals; the ones facing
        +Z are the faces 1 and 3 to n+1, and the ones facing -Z are 2 and
        n+2 to 2n. The internal vertex at the angle of the external vertex
        k+1 (at k*360/n degrees, k = 0 .. n-1) is n+1+((n-k) mod n).
      - without a hole (a disk): each face is split into a fan of `n`
        triangles towards a new vertex at its center (see `splitFaceIntoFan`);
        the two faces get distinct, coincident center vertices (n+1 for
        face 1 and n+2 for face 2).
    @param ri internal radius, or 0 for a disk
    @param re external radius
    @param h height of the plane of the lamina
    @param n number of sides of the circles (at least 3)
    @param withQuadrilaterals false for two faces with a hole, true to split
    them in quadrilaterals (triangles for a disk)
    @return the lamina; face 1 is the one facing +Z (it is the one to sweep)
    */
    public static PolyhedralBoundedSolid createWasherLamina(double ri,
        double re, double h, int n, boolean withQuadrilaterals)
    {
        PolyhedralBoundedSolid solid;

        solid = PolyhedralBoundedSolidEulerConstructions.createCircularLamina(
            0.0, 0.0, re, h, n);
        if ( ri <= 0.0 ) {
            if ( withQuadrilaterals ) {
                splitFaceIntoFan(solid, 1, new Vector3Dd(0.0, 0.0, h));
                splitFaceIntoFan(solid, 2, new Vector3Dd(0.0, 0.0, h));
                PolyhedralBoundedSolidValidationEngine.validateIntermediate(solid);
            }
            return solid;
        }

        int firstInternal = solid.getMaxVertexId() + 1;
        PolyhedralBoundedSolidEulerOperators.smev(solid, 1, 1, firstInternal,
            new Vector3Dd(ri, 0.0, h));
        PolyhedralBoundedSolidEulerOperators.kemr(solid, 1, 1,
            1, firstInternal, firstInternal, 1);
        // The internal circle goes clockwise (seen from +Z), against the
        // external one, so the disk it encloses is split as the new face
        PolyhedralBoundedSolidEulerConstructions.addArcToExistingFace(solid, 1,
            firstInternal, 0.0, 0.0, ri, h, 360.0, 360.0 / n, n - 1);
        int lastInternal = solid.getMaxVertexId();
        int diskFace = solid.getMaxFaceId() + 1;
        PolyhedralBoundedSolidEulerOperators.smef(solid, 1, lastInternal,
            firstInternal, diskFace);
        PolyhedralBoundedSolidEulerOperators.kfmrh(solid, 2, diskFace);
        if ( withQuadrilaterals ) {
            splitWasherIntoQuadrilaterals(solid, n);
        }
        PolyhedralBoundedSolidValidationEngine.validateIntermediate(solid);
        return solid;
    }

    /**
    Splits face `faceId` of `solid`, a single loop polygon, into a fan of
    triangles towards a new vertex at `center` (`smev` from the first vertex
    of the loop, then `mef` around it). The face keeps one of the triangles
    and the others get new face ids, in the order of the loop.
    @param solid solid holding the face
    @param faceId id of the face to split
    @param center position of the new center vertex
    @return the id of the new center vertex, or 0 if the face does not
    exist or is not a polygon of at least three vertices
    */
    public static int splitFaceIntoFan(PolyhedralBoundedSolid solid,
        int faceId, Vector3Dd center)
    {
        _PolyhedralBoundedSolidFace face = solid.findFace(faceId);
        if ( face == null || face.boundariesList.size() <= 0 ) {
            return 0;
        }

        _PolyhedralBoundedSolidLoop loop = face.boundariesList.get(0);
        _PolyhedralBoundedSolidHalfEdge start = loop.boundaryStartHalfEdge;
        if ( start == null ) {
            return 0;
        }

        ArrayList<Integer> ringVertexIds = new ArrayList<Integer>();
        _PolyhedralBoundedSolidHalfEdge he = start;
        do {
            ringVertexIds.add(Integer.valueOf(he.startingVertex.id));
            he = he.next();
        } while ( he != start );

        if ( ringVertexIds.size() < 3 ) {
            return 0;
        }

        int centerVertexId = solid.getMaxVertexId() + 1;
        PolyhedralBoundedSolidEulerOperators.smev(solid, faceId,
            ringVertexIds.get(0).intValue(), centerVertexId, center);

        int i;
        for ( i = 0; i < ringVertexIds.size() - 2; i++ ) {
            PolyhedralBoundedSolidEulerOperators.mef(solid, faceId, faceId,
                centerVertexId,
                ringVertexIds.get(i).intValue(),
                ringVertexIds.get(i+1).intValue(),
                ringVertexIds.get(i+2).intValue(),
                solid.getMaxFaceId() + 1);
        }

        PolyhedralBoundedSolidEulerOperators.mef(solid, faceId, faceId,
            centerVertexId,
            ringVertexIds.get(ringVertexIds.size()-2).intValue(),
            ringVertexIds.get(ringVertexIds.size()-1).intValue(),
            ringVertexIds.get(0).intValue(),
            solid.getMaxFaceId() + 1);
        return centerVertexId;
    }

    /**
    @return the id of the face among `candidateFaceIds` that has both
    vertices on one of its loops, or 0 if there is none
    */
    private static int findFaceWithVertices(PolyhedralBoundedSolid solid,
        ArrayList<Integer> candidateFaceIds, int vertexId1, int vertexId2)
    {
        int i;
        int j;

        for ( i = 0; i < candidateFaceIds.size(); i++ ) {
            _PolyhedralBoundedSolidFace face =
                solid.findFace(candidateFaceIds.get(i).intValue());
            if ( face == null ) {
                continue;
            }
            for ( j = 0; j < face.boundariesList.size(); j++ ) {
                _PolyhedralBoundedSolidLoop loop = face.boundariesList.get(j);
                if ( loop.firstHalfEdgeAtVertex(vertexId1) != null &&
                     loop.firstHalfEdgeAtVertex(vertexId2) != null ) {
                    return face.id;
                }
            }
        }
        return 0;
    }

    /**
    Splits both faces of a washer lamina into quadrilaterals (see
    `createWasherLamina`).

    The external circle has the vertices 1..n (vertex k+1 at k*360/n degrees)
    and the internal one n+1..2n, clockwise: vertex n+1+j is at -j*360/n
    degrees, so the external vertex k+1 pairs with the internal vertex
    n+1+((n-k) mod n), at its same angle.
    */
    private static void splitWasherIntoQuadrilaterals(
        PolyhedralBoundedSolid solid, int n)
    {
        int nextFaceId = 3;
        int side;
        int k;

        for ( side = 1; side <= 2; side++ ) {
            ArrayList<Integer> sideFaces = new ArrayList<Integer>();
            sideFaces.add(Integer.valueOf(side));
            _PolyhedralBoundedSolidFace face = solid.findFace(side);
            PolyhedralBoundedSolidEulerOperators.lmekr(solid,
                face.findHalfEdge(1), face.findHalfEdge(n + 1));
            for ( k = 1; k < n; k++ ) {
                int externalId = k + 1;
                int internalId = n + 1 + (n - k);
                int faceId = findFaceWithVertices(solid, sideFaces, externalId,
                    internalId);
                PolyhedralBoundedSolidEulerOperators.smef(solid, faceId,
                    externalId, internalId, nextFaceId);
                sideFaces.add(Integer.valueOf(nextFaceId));
                nextFaceId++;
            }
        }
    }

    /**
    The tube is built as a translational sweep ([MANT1988] 12.3.1, program
    12.4) of a washer lamina (see `createWasherLamina`): its face 1, with
    both loops, is extruded `height` units along +Z. A washer is just the
    lamina.
    */
    private PolyhedralBoundedSolid buildPolyhedralBoundedSolid(int nsides)
    {
        PolyhedralBoundedSolid solid = createWasherLamina(internalRadius,
            externalRadius, 0.0, nsides);

        if ( !isWasher() ) {
            Matrix4x4d T = new Matrix4x4d().translation(0.0, 0.0, height);
            PolyhedralBoundedSolidEulerConstructions.translationalSweepExtrudeFace(
                solid, solid.findFace(1), T);
        }
        return solid;
    }
}
