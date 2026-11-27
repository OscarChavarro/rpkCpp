//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

package vsdk.toolkit.environment.geometry.volume;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.PolyhedralBoundedSolidEulerOperators;
import java.io.Serial;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.numerics.FloatingInterval;
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

public class Box extends Solid {
    @Serial private static final long serialVersionUID = 20060502L;
    private static final Vector3Dd NORMAL_POS_Z = new Vector3Dd(0, 0, 1);
    private static final Vector3Dd NORMAL_NEG_Z = new Vector3Dd(0, 0, -1);
    private static final Vector3Dd NORMAL_POS_Y = new Vector3Dd(0, 1, 0);
    private static final Vector3Dd NORMAL_NEG_Y = new Vector3Dd(0, -1, 0);
    private static final Vector3Dd NORMAL_POS_X = new Vector3Dd(1, 0, 0);
    private static final Vector3Dd NORMAL_NEG_X = new Vector3Dd(-1, 0, 0);
    private static final Vector3Dd TANGENT_POS_Y = new Vector3Dd(0, 1, 0);
    private static final Vector3Dd TANGENT_NEG_Y = new Vector3Dd(0, -1, 0);
    private static final Vector3Dd TANGENT_NEG_X = new Vector3Dd(-1, 0, 0);
    private static final Vector3Dd TANGENT_POS_X = new Vector3Dd(1, 0, 0);
    private static final Vector3Dd ZERO_VECTOR = new Vector3Dd();

    private final Vector3Dd[] bounds = new Vector3Dd[2];

    public Box(double dx, double dy, double dz) {
        this(new Vector3Dd(dx,dy,dz));
    }

    public Box(Vector3Dd s) {
        bounds[0] = new Vector3Dd(-s.x()/2,-s.y()/2,-s.z()/2);
        bounds[1] = new Vector3Dd(s.x()/2,s.y()/2,s.z()/2);
        addControlSpecifications();
    }

    public Box(Vector3Dd minimum, Vector3Dd maximum) {
        bounds[0] = minimum;
        bounds[1] = maximum;
        addControlSpecifications();
    }

    /**
    The state edited is the size along each axis; the center is kept (moving
    the box is a transformation of its body, not a change of the box).
    */
    private void addControlSpecifications() {
        getControlSpecifications().add("double;sizeX;(0, INFINITE)");
        getControlSpecifications().add("double;sizeY;(0, INFINITE)");
        getControlSpecifications().add("double;sizeZ;(0, INFINITE)");
    }

    public Vector3Dd[] getRawBounds() { return bounds; }
    public Vector3Dd getCenter() { return bounds[0].multiply(0.5).add(bounds[1].multiply(0.5)); }
    @Override public Geometry copyGeometry() { return new Box(bounds[0], bounds[1]); }

    @Override
    public void getBounds(GeometryBounds result) {
        final Vector3Dd size = getSize();
        if (!Double.isFinite(size.x()) || !Double.isFinite(size.y()) ||
            !Double.isFinite(size.z()) || size.x() <= 0 || size.y() <= 0 || size.z() <= 0) {
            result.setIndeterminate(); return;
        }
        result.setFinite(bounds[0], bounds[1]);
    }

    @Override
    public GeometryContainment classifyPoint(Vector3Dd point, double tolerance) {
        if (Double.isFinite(tolerance) && tolerance > 0) return classifyPointCertified(point, tolerance);
        final double[] minimum = {bounds[0].x(), bounds[0].y(), bounds[0].z()};
        final double[] maximum = {bounds[1].x(), bounds[1].y(), bounds[1].z()};
        final double[] coordinates = {point.x(), point.y(), point.z()};
        if (!Double.isFinite(tolerance) || tolerance < 0) return GeometryContainment.InvalidInput;
        boolean outside = false;
        boolean onFace = false;
        for (int i = 0; i < 3; ++i) {
            final double dimension = maximum[i] - minimum[i];
            if (!Double.isFinite(dimension) || dimension <= 0 ||
                !Double.isFinite(coordinates[i])) return GeometryContainment.InvalidInput;
            if (!Double.isFinite(minimum[i]) || !Double.isFinite(maximum[i]) ||
                dimension / 2 == 0) return GeometryContainment.Indeterminate;
            if (coordinates[i] < minimum[i] || coordinates[i] > maximum[i]) outside = true;
            else if (coordinates[i] == minimum[i] || coordinates[i] == maximum[i]) onFace = true;
        }
        if (outside) return GeometryContainment.Outside;
        return onFace ? GeometryContainment.Boundary : GeometryContainment.Inside;
    }

    @Override
    public GeometryContainment classifyBoxCertified(Vector3Dd center, Vector3Dd halfWidths) {
        final double[] minimum = {bounds[0].x(), bounds[0].y(), bounds[0].z()};
        final double[] maximum = {bounds[1].x(), bounds[1].y(), bounds[1].z()};
        final double[] c = {center.x(), center.y(), center.z()};
        final double[] h = {halfWidths.x(), halfWidths.y(), halfWidths.z()};
        for (int i = 0; i < 3; ++i) {
            if (!Double.isFinite(c[i]) || !Double.isFinite(h[i]) || h[i] < 0)
                return GeometryContainment.InvalidInput;
            if (!Double.isFinite(minimum[i]) || !Double.isFinite(maximum[i]))
                return GeometryContainment.Indeterminate;
            if (!(minimum[i] < maximum[i])) return GeometryContainment.InvalidInput;
        }
        if (!FloatingInterval.isSupportedEnvironment()) return GeometryContainment.Unsupported;
        boolean inside = true;
        for (int i = 0; i < 3; ++i) {
            final FloatingInterval range = FloatingInterval.around(c[i], h[i]);
            if (!range.isValid()) return GeometryContainment.Indeterminate;
            if (range.getUpper() < minimum[i] || range.getLower() > maximum[i]) return GeometryContainment.Outside;
            if (!(range.getLower() > minimum[i] && range.getUpper() < maximum[i])) inside = false;
        }
        return inside ? GeometryContainment.Inside : GeometryContainment.Indeterminate;
    }

    @Override
    public GeometryContainment classifyPointCertified(Vector3Dd point, double tolerance) {
        final double[] minimum = {bounds[0].x(), bounds[0].y(), bounds[0].z()};
        final double[] maximum = {bounds[1].x(), bounds[1].y(), bounds[1].z()};
        final double[] coordinates = {point.x(), point.y(), point.z()};
        if (!Double.isFinite(tolerance) || tolerance < 0) return GeometryContainment.InvalidInput;
        for (int i = 0; i < 3; ++i) {
            if (!Double.isFinite(coordinates[i])) return GeometryContainment.InvalidInput;
            if (!Double.isFinite(minimum[i]) || !Double.isFinite(maximum[i]))
                return GeometryContainment.Indeterminate;
            if (!(minimum[i] < maximum[i])) return GeometryContainment.InvalidInput;
        }
        if (!FloatingInterval.isSupportedEnvironment()) return GeometryContainment.Unsupported;
        boolean inside = true;
        for (int i = 0; i < 3; ++i)
            if (coordinates[i] < minimum[i] || coordinates[i] > maximum[i]) inside = false;
        if (inside) {
            // The distance to the boundary is the smallest face distance.
            boolean deep = true, near = false;
            for (int i = 0; i < 3; ++i) {
                if (coordinates[i] == minimum[i] || coordinates[i] == maximum[i]) { near = true; deep = false; continue; }
                final FloatingInterval low = FloatingInterval.exact(coordinates[i]).subtract(FloatingInterval.exact(minimum[i]));
                final FloatingInterval high = FloatingInterval.exact(maximum[i]).subtract(FloatingInterval.exact(coordinates[i]));
                if (!low.isValid() || !high.isValid()) return GeometryContainment.Indeterminate;
                if (!(low.getLower() > tolerance) || !(high.getLower() > tolerance)) deep = false;
                if (low.getUpper() <= tolerance || high.getUpper() <= tolerance) near = true;
            }
            if (deep) return GeometryContainment.Inside;
            return near ? GeometryContainment.Boundary : GeometryContainment.Indeterminate;
        }
        FloatingInterval squared = FloatingInterval.exact(0);
        for (int i = 0; i < 3; ++i) {
            if (coordinates[i] < minimum[i])
                squared = squared.add(FloatingInterval.exact(minimum[i]).subtract(FloatingInterval.exact(coordinates[i])).square());
            else if (coordinates[i] > maximum[i])
                squared = squared.add(FloatingInterval.exact(coordinates[i]).subtract(FloatingInterval.exact(maximum[i])).square());
        }
        final FloatingInterval distance = squared.squareRoot();
        if (!distance.isValid()) return GeometryContainment.Indeterminate;
        if (distance.getLower() > tolerance) return GeometryContainment.Outside;
        return distance.getUpper() <= tolerance ? GeometryContainment.Boundary : GeometryContainment.Indeterminate;
    }

    /**
    Mathematical operations on arbitrary axis-aligned bounds. Boundary
    inclusion and the face selection tolerance are chosen by the caller.
    */
    public static boolean containsBounds(Vector3Dd point, Vector3Dd minimum,
        Vector3Dd maximum)
    {
        return !(point.x() < minimum.x() || point.x() > maximum.x() ||
            point.y() < minimum.y() || point.y() > maximum.y() ||
            point.z() < minimum.z() || point.z() > maximum.z());
    }

    public static Vector3Dd normalForBounds(Vector3Dd point, Vector3Dd minimum,
        Vector3Dd maximum, double tolerance)
    {
        if (Math.abs(point.x() - maximum.x()) < tolerance) return new Vector3Dd(1,0,0);
        if (Math.abs(point.x() - minimum.x()) < tolerance) return new Vector3Dd(-1,0,0);
        if (Math.abs(point.y() - maximum.y()) < tolerance) return new Vector3Dd(0,1,0);
        if (Math.abs(point.y() - minimum.y()) < tolerance) return new Vector3Dd(0,-1,0);
        if (Math.abs(point.z() - maximum.z()) < tolerance) return new Vector3Dd(0,0,1);
        if (Math.abs(point.z() - minimum.z()) < tolerance) return new Vector3Dd(0,0,-1);
        return new Vector3Dd(0,0,0);
    }

    /** `entry`, `exit` and `faceOverlap` are one-element in/out holders. */
    private static RayFirstHitStatus clipAxis(double origin, double direction,
        double minimum, double maximum, double parallelTolerance, boolean checkArithmetic,
        double[] entry, double[] exit, boolean[] faceOverlap)
    {
        if (direction < -parallelTolerance || direction > parallelTolerance) {
            final double farBound = direction < 0 ? minimum : maximum;
            final double nearBound = direction < 0 ? maximum : minimum;
            final double farT = (farBound - origin) / direction;
            if (!checkArithmetic && farT < entry[0]) return RayFirstHitStatus.Miss;
            final double nearT = (nearBound - origin) / direction;
            if (checkArithmetic && (!Double.isFinite(farT) ||
                !Double.isFinite(nearT) || farT == nearT))
                return RayFirstHitStatus.Indeterminate;
            if (farT < entry[0]) return RayFirstHitStatus.Miss;
            if (farT <= exit[0]) exit[0] = farT;
            if (nearT >= entry[0]) {
                if (nearT > exit[0]) return RayFirstHitStatus.Miss;
                entry[0] = nearT;
            }
            return RayFirstHitStatus.Hit;
        }
        if (origin < minimum || origin > maximum) return RayFirstHitStatus.Miss;
        if (origin == minimum || origin == maximum) faceOverlap[0] = true;
        return RayFirstHitStatus.Hit;
    }

    /**
    Intersect arbitrary axis-aligned bounds with an initial parameter interval.
    The caller selects its parallel threshold and whether arithmetic failures
    are reported; a parallel ray on a face is reported separately.
    `entry`, `exit` and `faceOverlap` are one-element in/out holders.
    */
    public static RayFirstHitStatus intersectBounds(Vector3Dd origin,
        Vector3Dd direction, Vector3Dd minimum, Vector3Dd maximum,
        double parallelTolerance, boolean checkArithmetic, double[] entry, double[] exit,
        boolean[] faceOverlap)
    {
        faceOverlap[0] = false;
        if (checkArithmetic &&
            (!Double.isFinite(origin.x()) || !Double.isFinite(origin.y()) ||
             !Double.isFinite(origin.z()) || !Double.isFinite(direction.x()) ||
             !Double.isFinite(direction.y()) || !Double.isFinite(direction.z()) ||
             (direction.x() == 0 && direction.y() == 0 && direction.z() == 0) ||
             !Double.isFinite(minimum.x()) || !Double.isFinite(minimum.y()) ||
             !Double.isFinite(minimum.z()) || !Double.isFinite(maximum.x()) ||
             !Double.isFinite(maximum.y()) || !Double.isFinite(maximum.z()) ||
             minimum.x() >= maximum.x() || minimum.y() >= maximum.y() || minimum.z() >= maximum.z() ||
             !Double.isFinite(parallelTolerance) || parallelTolerance < 0 ||
             Double.isNaN(entry[0]) || Double.isNaN(exit[0]) || entry[0] > exit[0]))
            return RayFirstHitStatus.InvalidInput;
        RayFirstHitStatus result = clipAxis(origin.x(), direction.x(), minimum.x(), maximum.x(),
            parallelTolerance, checkArithmetic, entry, exit, faceOverlap);
        if (result != RayFirstHitStatus.Hit) return result;
        result = clipAxis(origin.y(), direction.y(), minimum.y(), maximum.y(),
            parallelTolerance, checkArithmetic, entry, exit, faceOverlap);
        if (result != RayFirstHitStatus.Hit) return result;
        return clipAxis(origin.z(), direction.z(), minimum.z(), maximum.z(),
            parallelTolerance, checkArithmetic, entry, exit, faceOverlap);
    }

    /**
    Slab intersection in the original affine ray parameterization. A ray that
    overlaps a face for an interval has no finite sequence of surface events.
    */
    private RayFirstHitStatus clipRay(Ray ray, RayCrossingQuery query,
        double[] entry, double[] exit) {
        final Vector3Dd size = getSize();
        final double[] dimensions = {size.x(), size.y(), size.z()};
        final double[] origins = {ray.getOrigin().x(), ray.getOrigin().y(), ray.getOrigin().z()};
        final double[] directions = {ray.getDirection().x(), ray.getDirection().y(), ray.getDirection().z()};
        if (Double.isNaN(query.minT) || Double.isNaN(query.maxT) || query.minT > query.maxT ||
            !Double.isFinite(query.minimumChordSquared) || query.minimumChordSquared < 0 ||
            Double.isNaN(query.minimumExteriorApproach)) return RayFirstHitStatus.InvalidInput;
        if (query.minimumChordSquared != 0 ||
            query.minimumExteriorApproach != Double.NEGATIVE_INFINITY) return RayFirstHitStatus.Unsupported;
        if (directions[0] == 0 && directions[1] == 0 && directions[2] == 0) return RayFirstHitStatus.InvalidInput;
        for (int i = 0; i < 3; ++i) {
            if (!Double.isFinite(dimensions[i]) || dimensions[i] <= 0 ||
                !Double.isFinite(origins[i]) || !Double.isFinite(directions[i]))
                return RayFirstHitStatus.InvalidInput;
        }
        entry[0] = Double.NEGATIVE_INFINITY;
        exit[0] = Double.POSITIVE_INFINITY;
        final Vector3Dd half = new Vector3Dd(size.x()/2, size.y()/2, size.z()/2);
        if (half.x() == 0 || half.y() == 0 || half.z() == 0)
            return RayFirstHitStatus.Indeterminate;
        boolean[] faceOverlap = {false};
        final RayFirstHitStatus clipped = intersectBounds(ray.getOrigin(), ray.getDirection(),
            bounds[0], bounds[1], 0, true, entry, exit, faceOverlap);
        if (clipped != RayFirstHitStatus.Hit) return clipped;
        if (exit[0] < query.minT || entry[0] > query.maxT) return RayFirstHitStatus.Miss;
        if (faceOverlap[0] && entry[0] < exit[0]) return RayFirstHitStatus.Indeterminate;
        return RayFirstHitStatus.Hit;
    }

    @Override
    public RayCrossingSupport emitRayCrossings(Ray ray, RayCrossingQuery query,
        RayCrossingSink sink) {
        double[] entryH = new double[1], exitH = new double[1];
        final RayFirstHitStatus status = clipRay(ray, query, entryH, exitH);
        if (status == RayFirstHitStatus.Miss) return RayCrossingSupport.Supported;
        if (status == RayFirstHitStatus.Unsupported) return RayCrossingSupport.Unsupported;
        if (status == RayFirstHitStatus.InvalidInput) return RayCrossingSupport.InvalidInput;
        if (status != RayFirstHitStatus.Hit) return RayCrossingSupport.Indeterminate;
        final double entry = entryH[0], exit = exitH[0];
        final Vector3Dd first = ray.getOrigin().add(ray.getDirection().multiply(entry));
        final Vector3Dd last = ray.getOrigin().add(ray.getDirection().multiply(exit));
        if (!Double.isFinite(first.x()) || !Double.isFinite(first.y()) ||
            !Double.isFinite(first.z()) || !Double.isFinite(last.x()) ||
            !Double.isFinite(last.y()) || !Double.isFinite(last.z()))
            return RayCrossingSupport.Indeterminate;
        if (entry >= query.minT && entry <= query.maxT)
            sink.crossing(new RayCrossing(entry, this, entry == exit ? RayCrossingKind.Tangent : RayCrossingKind.Entering));
        if (exit != entry && exit >= query.minT && exit <= query.maxT)
            sink.crossing(new RayCrossing(exit, this, RayCrossingKind.Leaving));
        return RayCrossingSupport.Supported;
    }

    @Override
    public RayFirstHitStatus queryFirstHit(Ray ray, RayCrossingQuery query,
        RayHit result, RayBoundary boundary) {
        if (result != null) result.clear();
        if (boundary != null) boundary.assign(new RayBoundary());
        double[] entryH = new double[1], exitH = new double[1];
        final RayFirstHitStatus status = clipRay(ray, query, entryH, exitH);
        if (status != RayFirstHitStatus.Hit) return status;
        final double entry = entryH[0], exit = exitH[0];
        final double t = entry >= query.minT ? entry : exit;
        if (t > query.maxT) return RayFirstHitStatus.Miss;
        final Vector3Dd point = ray.getOrigin().add(ray.getDirection().multiply(t));
        if (!Double.isFinite(point.x()) || !Double.isFinite(point.y()) ||
            !Double.isFinite(point.z())) return RayFirstHitStatus.Indeterminate;
        if (boundary != null) boundary.assign(new RayBoundary(t, this));
        if (result != null) {
            result.setHitDistance(t);
            if (result.shouldStoreRay()) result.setRay(ray.withT(t));
            if (result.needsAnySurfaceData()) doExtraInformation(ray, t, result);
        }
        return RayFirstHitStatus.Hit;
    }

    @Override
    public RayFirstHitStatus querySurfaceDetail(Ray ray, RayBoundary boundary,
        RayHit hit) {
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

    @Override
    public RaySolidQueryStatus classifyRay(Ray ray, RayCrossingQuery query,
        RaySolidClassification result) {
        result.clear();
        if (query.minT >= query.maxT) return RaySolidQueryStatus.InvalidInput;
        double[] entryH = new double[1], exitH = new double[1];
        final RayFirstHitStatus status = clipRay(ray, query, entryH, exitH);
        if (status == RayFirstHitStatus.Unsupported) return RaySolidQueryStatus.Unsupported;
        if (status == RayFirstHitStatus.InvalidInput) return RaySolidQueryStatus.InvalidInput;
        if (status == RayFirstHitStatus.Indeterminate) return RaySolidQueryStatus.Indeterminate;
        final double entry = entryH[0], exit = exitH[0];
        final boolean intersectsInterior = status == RayFirstHitStatus.Hit && entry < exit;
        if (!result.insideSegments.add(intersectsInterior && query.minT > entry && query.minT <= exit))
            return RaySolidQueryStatus.Indeterminate;
        if (intersectsInterior && entry >= query.minT && entry <= query.maxT) {
            if (!result.boundaries.add(new RaySolidBoundary(entry, this)) || !result.insideSegments.add(true)) {
                result.clear(); return RaySolidQueryStatus.Indeterminate;
            }
        }
        if (intersectsInterior && exit >= query.minT && exit <= query.maxT) {
            if (!result.boundaries.add(new RaySolidBoundary(exit, this)) || !result.insideSegments.add(false)) {
                result.clear(); return RaySolidQueryStatus.Indeterminate;
            }
        }
        return RaySolidQueryStatus.Supported;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doIntersectionFirstHit.
    @param inOutRay
    @return true if given ray intersects current Box
    */
    public Ray
    doIntersectionFirstHit(Ray inOutRay) {
        RayHit hit = new RayHit();
        if ( doIntersectionFirstHit(inOutRay, hit) && hit.getRay() != null ) {
            return hit.getRay();
        }
        return null;
    }

    @Override
    public boolean doIntersectionFirstHit(Ray inRay, RayHit outHit)
    {
        double minT = 1e308;
        int hitPlane = 0;
        final Vector3Dd size = getSize();
        final Vector3Dd center = getCenter();
        double x2 = size.x()/2, y2 = size.y()/2, z2 = size.z()/2;
        double ox = inRay.getOrigin().x()-center.x(), oy = inRay.getOrigin().y()-center.y(), oz = inRay.getOrigin().z()-center.z();
        double dx = inRay.getDirection().x(), dy = inRay.getDirection().y(), dz = inRay.getDirection().z();

        if (Math.abs(dz) > VSDK.EPSILON) {
            double t = (z2 - oz) / dz;
            if (t > -VSDK.EPSILON) {
                double cx = ox + dx*t, cy = oy + dy*t;
                if (cx >= -x2 && cx <= x2 && cy >= -y2 && cy <= y2) { minT = t; hitPlane = 1; }
            }
        }
        if (Math.abs(dz) > VSDK.EPSILON) {
            double t = (-z2 - oz) / dz;
            if (t > -VSDK.EPSILON && t < minT) {
                double cx = ox + dx*t, cy = oy + dy*t;
                if (cx >= -x2 && cx <= x2 && cy >= -y2 && cy <= y2) { minT = t; hitPlane = 2; }
            }
        }
        if (Math.abs(dy) > VSDK.EPSILON) {
            double t = (y2 - oy) / dy;
            if (t > -VSDK.EPSILON && t < minT) {
                double cx = ox + dx*t, cz = oz + dz*t;
                if (cx >= -x2 && cx <= x2 && cz >= -z2 && cz <= z2) { minT = t; hitPlane = 3; }
            }
        }
        if (Math.abs(dy) > VSDK.EPSILON) {
            double t = (-y2 - oy) / dy;
            if (t > -VSDK.EPSILON && t < minT) {
                double cx = ox + dx*t, cz = oz + dz*t;
                if (cx >= -x2 && cx <= x2 && cz >= -z2 && cz <= z2) { minT = t; hitPlane = 4; }
            }
        }
        if (Math.abs(dx) > VSDK.EPSILON) {
            double t = (x2 - ox) / dx;
            if (t > -VSDK.EPSILON && t < minT) {
                double cy = oy + dy*t, cz = oz + dz*t;
                if (cy >= -y2 && cy <= y2 && cz >= -z2 && cz <= z2) { minT = t; hitPlane = 5; }
            }
        }
        if (Math.abs(dx) > VSDK.EPSILON) {
            double t = (-x2 - ox) / dx;
            if (t > -VSDK.EPSILON && t < minT) {
                double cy = oy + dy*t, cz = oz + dz*t;
                if (cy >= -y2 && cy <= y2 && cz >= -z2 && cz <= z2) { minT = t; hitPlane = 6; }
            }
        }

        if (minT == 1e308) return false;
        if (outHit == null) return true;

        if (outHit.shouldStoreRay() || outHit.needsAnySurfaceData()) outHit.setRay(inRay.withT(minT));
        else outHit.setHitDistance(minT);

        if (outHit.needsAnySurfaceData()) {
            double hitX = ox + dx*minT, hitY = oy + dy*minT, hitZ = oz + dz*minT;
            if (outHit.needsPoint()) outHit.point = new Vector3Dd(hitX, hitY, hitZ).add(center);
            if (outHit.needsTextureCoordinates()) {
                outHit.u = 0; outHit.v = 0;
                switch (hitPlane) {
                    case 1: outHit.u = hitY / size.y() - 0.5; outHit.v = 1-(hitX / size.x() - 0.5); break;
                    case 2: outHit.u = hitY / size.y() - 0.5; outHit.v = hitX / size.x() - 0.5; break;
                    case 3: outHit.u = 1-(hitX / size.x() - 0.5); outHit.v = hitZ / size.z() - 0.5; break;
                    case 4: outHit.u = hitX / size.x() - 0.5; outHit.v = hitZ / size.z() - 0.5; break;
                    case 5: outHit.u = hitY / size.y() - 0.5; outHit.v = hitZ / size.z() - 0.5; break;
                    case 6: outHit.u = 1-(hitY / size.y() - 0.5); outHit.v = hitZ / size.z() - 0.5; break;
                    default: break;
                }
            }
            if (outHit.needsNormal()) outHit.normal = planeNormal(hitPlane);
            if (outHit.needsTangent()) outHit.tangent = planeTangent(hitPlane);
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
    public void
    doExtraInformation(Ray inRay, double inT,
                                  RayHit outData) {
        if ( outData == null ) return;
        outData.beginDetailEvaluation();
        double hitX = inRay.getOrigin().x() + inRay.getDirection().x()*inT;
        double hitY = inRay.getOrigin().y() + inRay.getDirection().y()*inT;
        double hitZ = inRay.getOrigin().z() + inRay.getDirection().z()*inT;
        final Vector3Dd center = getCenter();
        int hitPlane = classifyHitPlane(hitX-center.x(), hitY-center.y(), hitZ-center.z());

        if ( outData.needsPoint() ) {
            outData.point = new Vector3Dd(hitX, hitY, hitZ);
            outData.markDetailAvailable(RayHit.DETAIL_POINT);
        }
        if ( outData.needsNormal() ) {
            outData.normal = planeNormal(hitPlane);
            outData.markDetailAvailable(RayHit.DETAIL_NORMAL);
        }
        if ( outData.needsTangent() ) {
            outData.tangent = planeTangent(hitPlane);
            outData.markDetailAvailable(RayHit.DETAIL_TANGENT);
        }

        // Java-only (pre-existing, cpp computes no UV here); kept, not marked
        // available, in box local coordinates relative to its center.
        final Vector3Dd size = getSize();
        hitX -= center.x(); hitY -= center.y(); hitZ -= center.z();
        if ( outData.needsTextureCoordinates() ) {
            switch ( hitPlane ) {
              case 1:
                outData.u = hitY / size.y() - 0.5;
                outData.v = 1-(hitX / size.x() - 0.5);
                break;
              case 2:
                outData.u = hitY / size.y() - 0.5;
                outData.v = hitX / size.x() - 0.5;
                break;
              case 3:
                outData.u = 1-(hitX / size.x() - 0.5);
                outData.v = hitZ / size.z() - 0.5;
                break;
              case 4:
                outData.u = hitX / size.x() - 0.5;
                outData.v = hitZ / size.z() - 0.5;
                break;
              case 5:
                outData.u = hitY / size.y() - 0.5;
                outData.v = hitZ / size.z() - 0.5;
                break;
              case 6:
                outData.u = 1-(hitY / size.y() - 0.5);
                outData.v = hitZ / size.z() - 0.5;
                break;
              default:
                outData.u = 0;
                outData.v = 0;
                break;
            }
        }
    }

    private static Vector3Dd planeNormal(int hitPlane)
    {
        switch ( hitPlane ) {
          case 1: return NORMAL_POS_Z;
          case 2: return NORMAL_NEG_Z;
          case 3: return NORMAL_POS_Y;
          case 4: return NORMAL_NEG_Y;
          case 5: return NORMAL_POS_X;
          case 6: return NORMAL_NEG_X;
          default: return ZERO_VECTOR;
        }
    }

    private static Vector3Dd planeTangent(int hitPlane)
    {
        switch ( hitPlane ) {
          case 1:
          case 2:
          case 5:
            return TANGENT_POS_Y;
          case 3:
            return TANGENT_NEG_X;
          case 4:
            return TANGENT_POS_X;
          case 6:
            return TANGENT_NEG_Y;
          default:
            return ZERO_VECTOR;
        }
    }

    private int classifyHitPlane(double x, double y, double z)
    {
        final Vector3Dd size = getSize();
        double x2 = size.x() / 2;
        double y2 = size.y() / 2;
        double z2 = size.z() / 2;
        double dxPlus = Math.abs(x - x2);
        double dxMinus = Math.abs(x + x2);
        double dyPlus = Math.abs(y - y2);
        double dyMinus = Math.abs(y + y2);
        double dzPlus = Math.abs(z - z2);
        double dzMinus = Math.abs(z + z2);

        double min = dzPlus;
        int plane = 1;
        if ( dzMinus < min ) { min = dzMinus; plane = 2; }
        if ( dyPlus < min ) { min = dyPlus; plane = 3; }
        if ( dyMinus < min ) { min = dyMinus; plane = 4; }
        if ( dxPlus < min ) { min = dxPlus; plane = 5; }
        if ( dxMinus < min ) { plane = 6; }
        return plane;
    }

    /**
    Check the general interface contract in superclass method
    Geometry.doContainmentTest. The box is centered at the origin, with its
    faces perpendicular to the axes.
    Java-parity legacy band: per-axis |p_i| within half_i +/- tolerance. It is
    intentionally not the Euclidean band of classifyPoint.
    @param p point to classify, in the space of the box
    @param distanceTolerance distance to the faces of the box under which
    the point is classified as `LIMIT`
    @return INSIDE, OUTSIDE or LIMIT constant value
    */
    @Override
    public int doContainmentTest(Vector3Dd p, double distanceTolerance)
    {
        final Vector3Dd size = getSize();
        final Vector3Dd local = p.subtract(getCenter());
        double[] half = { size.x() / 2, size.y() / 2, size.z() / 2 };
        double[] coordinates = { local.x(), local.y(), local.z() };
        boolean inside = true;

        for ( int i = 0; i < 3; i++ ) {
            double distance = Math.abs(coordinates[i]);
            if ( distance > half[i] + distanceTolerance ) {
                return OUTSIDE;
            }
            if ( distance >= half[i] - distanceTolerance ) {
                inside = false;
            }
        }
        return inside ? INSIDE : LIMIT;
    }

    @Override
    public double[] getMinMax()
    {
        return new double[] {bounds[0].x(),bounds[0].y(),bounds[0].z(),bounds[1].x(),bounds[1].y(),bounds[1].z()};
    }

    public Vector3Dd getSize()
    {
        return bounds[1].subtract(bounds[0]);
    }

    public void setSize(double dx, double dy, double dz) {
        setSize(new Vector3Dd(dx, dy, dz));
    }

    public void setSize(Vector3Dd s) {
        final Vector3Dd center = getCenter();
        final Vector3Dd half = s.multiply(0.5);
        bounds[0] = center.subtract(half); bounds[1] = center.add(half);
    }

    public double getSizeX() { return bounds[1].x() - bounds[0].x(); }

    public double getSizeY() { return bounds[1].y() - bounds[0].y(); }

    public double getSizeZ() { return bounds[1].z() - bounds[0].z(); }

    public void setSizeX(double dx) { setSize(getSize().withX(dx)); }

    public void setSizeY(double dy) { setSize(getSize().withY(dy)); }

    public void setSizeZ(double dz) { setSize(getSize().withZ(dz)); }

    @Override
    public PolyhedralBoundedSolid exportToPolyhedralBoundedSolid()
    {
        return buildPolyhedralBoundedSolid();
    }

    /**
    Current method creates a polyhedral boundary representation for
    current box, following the strategy for Euler operators presented
    at sections [MANT1988].9.3., [MANT1988].12.3.1., as depicted in
    figure [MANT1988].9.11. and following the structure of the program
    [MANT1988].12.4.
    */
    private PolyhedralBoundedSolid buildPolyhedralBoundedSolid()
    {
        PolyhedralBoundedSolid solid;
        solid = new PolyhedralBoundedSolid();
        PolyhedralBoundedSolidEulerOperators.mvfs(solid, 
            new Vector3Dd(bounds[0].x(), bounds[0].y(), bounds[0].z()), 1, 1);
        PolyhedralBoundedSolidEulerOperators.smev(solid, 1, 1, 4,
            new Vector3Dd(bounds[0].x(), bounds[1].y(), bounds[0].z()));
        PolyhedralBoundedSolidEulerOperators.smev(solid, 1, 4, 3,
            new Vector3Dd(bounds[1].x(), bounds[1].y(), bounds[0].z()));
        PolyhedralBoundedSolidEulerOperators.smev(solid, 1, 3, 2,
            new Vector3Dd(bounds[1].x(), bounds[0].y(), bounds[0].z()));
        PolyhedralBoundedSolidEulerOperators.mef(solid, 1, 1, 1, 4, 2, 3, 2);
        
        PolyhedralBoundedSolidEulerOperators.smev(solid, 1, 1, 5,
            new Vector3Dd(bounds[0].x(), bounds[0].y(), bounds[1].z()));
        PolyhedralBoundedSolidEulerOperators.smev(solid, 1, 2, 6,
            new Vector3Dd(bounds[1].x(), bounds[0].y(), bounds[1].z()));
        PolyhedralBoundedSolidEulerOperators.mef(solid, 1, 1, 5, 1, 6, 2, 3);
        PolyhedralBoundedSolidEulerOperators.smev(solid, 1, 3, 7,
            new Vector3Dd(bounds[1].x(), bounds[1].y(), bounds[1].z()));
        PolyhedralBoundedSolidEulerOperators.mef(solid, 1, 1, 6, 2, 7, 3, 4);
        PolyhedralBoundedSolidEulerOperators.smev(solid, 1, 4, 8,
            new Vector3Dd(bounds[0].x(), bounds[1].y(), bounds[1].z()));
        PolyhedralBoundedSolidEulerOperators.mef(solid, 1, 1, 7, 3, 8, 4, 5);
        PolyhedralBoundedSolidEulerOperators.mef(solid, 1, 1, 5, 6, 8, 4, 6);
        return solid;
    }
}
