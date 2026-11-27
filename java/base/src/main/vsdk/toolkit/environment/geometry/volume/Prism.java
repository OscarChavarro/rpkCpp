//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =
//= [ROTH1982] Roth, Scott D. "Ray Casting for Modeling Solids", Computer  =
//=     Graphics and Image Processing 18, 109-144, 1982.                    =

package vsdk.toolkit.environment.geometry.volume;

import java.io.Serial;
import java.util.ArrayList;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.linealAlgebra.Matrix4x4d;
import vsdk.toolkit.common.linealAlgebra.Vector2Dd;
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

/**
A right prism: a simple polygon (the base) on the plane z = 0, swept
`height` units along +Z. The base may be concave; it may not cross itself.
Its vertices are kept counterclockwise seen from +Z (a clockwise polygon is
reversed when it is given), so the walls follow the edges of the base.

The prism is a parametric solid: rays are intersected and classified
against its definition (no boundary representation is built), so it can be
an operand of the constructive solid geometry evaluated with the ray
classification of [ROTH1982] (`classifyRay`). The interior along a line is
computed as the intersection of the slab 0 <= z <= height with the parts of
the line, projected on the plane z = 0, inside the base; for a concave base
there can be several intervals.

The surfaces of the prism are numbered (`RayBoundary.surfaceIndex`):
`SURFACE_BOTTOM` (z = 0), `SURFACE_TOP` (z = height) and, for each edge i
of the base (from vertex i to vertex i+1), the wall `SURFACE_FIRST_WALL` + i.

For generic editors (as the scene editor) it exposes the number of sides of
the base, its circumradius (the largest distance from the Z axis to a vertex
of the base) and its height. Changing the number of sides replaces the base
by a regular polygon with the same circumradius; changing the circumradius
scales the base about the Z axis.
*/
public class Prism extends Solid {
    @Serial private static final long serialVersionUID = 20261126L;

    public static final int SURFACE_NONE = -1;
    public static final int SURFACE_BOTTOM = 0;
    public static final int SURFACE_TOP = 1;
    public static final int SURFACE_FIRST_WALL = 2;

    private final ArrayList<Vector2Dd> base;
    private double height;
    /// Cached from the base: true when it is a simple polygon of non-zero area
    private boolean validBase;
    private double minX;
    private double minY;
    private double maxX;
    private double maxY;

    /**
    Interval [start, end] of the parameter of a line, whose ends are on the
    given surfaces of the prism (`SURFACE_NONE` for an unbounded end).
    */
    private static final class Interval
    {
        double start;
        double end;
        int startSurface;
        int endSurface;

        Interval()
        {
            this(0, SURFACE_NONE, 0, SURFACE_NONE);
        }

        Interval(double start, int startSurface, double end, int endSurface)
        {
            this.start = start;
            this.end = end;
            this.startSurface = startSurface;
            this.endSurface = endSurface;
        }

        Interval(Interval other)
        {
            this(other.start, other.startSurface, other.end, other.endSurface);
        }
    }

    /** A crossing of a line with an edge of the base, on the plane z = 0. */
    private static final class EdgeCrossing
    {
        double lineParameter;
        int edge;

        EdgeCrossing(double t, int edge)
        {
            this.lineParameter = t;
            this.edge = edge;
        }
    }

    //= Construction and parameters ======================================

    /**
    Regular prism: the base is a regular polygon of `numberOfSides` vertices
    at distance `circumradius` from the Z axis, the first one on +X.
    @param numberOfSides number of vertices of the base (at least 3)
    @param circumradius distance from the Z axis to the vertices
    @param height length of the sweep along +Z
    */
    public Prism(int numberOfSides, double circumradius, double height)
    {
        this.base = new ArrayList<Vector2Dd>();
        this.height = height;
        this.validBase = false;
        this.minX = 0;
        this.minY = 0;
        this.maxX = 0;
        this.maxY = 0;
        int n = numberOfSides < 3 ? 3 : numberOfSides;
        ArrayList<Vector2Dd> polygon = new ArrayList<Vector2Dd>();
        for ( int i = 0; i < n; i++ ) {
            double angle = 2.0 * Math.PI * ((double)i) / ((double)n);
            polygon.add(new Vector2Dd(circumradius * Math.cos(angle),
                circumradius * Math.sin(angle)));
        }
        setBase(polygon);
        addControlSpecifications();
    }

    /**
    @param base vertices of a simple polygon on the plane z = 0, in either
    orientation (it is stored counterclockwise)
    @param height length of the sweep along +Z
    */
    public Prism(ArrayList<Vector2Dd> base, double height)
    {
        this.base = new ArrayList<Vector2Dd>();
        this.height = height;
        this.validBase = false;
        this.minX = 0;
        this.minY = 0;
        this.maxX = 0;
        this.maxY = 0;
        setBase(base);
        addControlSpecifications();
    }

    private void addControlSpecifications()
    {
        getControlSpecifications().add("int;numberOfSides;[3, INFINITE)");
        getControlSpecifications().add("double;circumradius;(0, INFINITE)");
        getControlSpecifications().add("double;height;(0, INFINITE)");
    }

    @Override
    public Geometry copyGeometry()
    {
        return new Prism(base, height);
    }

    /**
    @return the vertices of the base, counterclockwise seen from +Z. The
    list is owned by the prism: use `setBase` to change it.
    */
    public ArrayList<Vector2Dd> getBase()
    {
        return base;
    }

    /**
    @param value vertices of a simple polygon on the plane z = 0, in either
    orientation (they are copied, counterclockwise)
    */
    public void setBase(ArrayList<Vector2Dd> value)
    {
        // Shoelace formula: a clockwise polygon has negative area
        double twiceArea = 0.0;
        int n = value.size();
        for ( int i = 0; i < n; i++ ) {
            Vector2Dd a = value.get(i);
            Vector2Dd b = value.get((i + 1) % n);
            twiceArea += a.x * b.y - b.x * a.y;
        }
        base.clear();
        if ( twiceArea < 0.0 ) {
            for ( int i = n - 1; i >= 0; i-- ) {
                base.add(new Vector2Dd(value.get(i)));
            }
        }
        else {
            for ( int i = 0; i < n; i++ ) {
                base.add(new Vector2Dd(value.get(i)));
            }
        }
        updateBaseCache();
    }

    public double getHeight()
    {
        return height;
    }

    public void setHeight(double value)
    {
        height = value;
    }

    public int getNumberOfSides()
    {
        return base.size();
    }

    public void setNumberOfSides(int value)
    {
        int n = value < 3 ? 3 : value;
        double r = getCircumradius();
        if ( !(r > 0.0) || !Double.isFinite(r) ) {
            r = 1.0;
        }
        ArrayList<Vector2Dd> polygon = new ArrayList<Vector2Dd>();
        for ( int i = 0; i < n; i++ ) {
            double angle = 2.0 * Math.PI * ((double)i) / ((double)n);
            polygon.add(new Vector2Dd(r * Math.cos(angle), r * Math.sin(angle)));
        }
        setBase(polygon);
    }

    public double getCircumradius()
    {
        double r = 0.0;
        for ( int i = 0; i < base.size(); i++ ) {
            double d = base.get(i).length();
            if ( d > r ) {
                r = d;
            }
        }
        return r;
    }

    public void setCircumradius(double value)
    {
        double r = getCircumradius();
        if ( !(r > 0.0) ) {
            return;
        }
        double factor = value / r;
        ArrayList<Vector2Dd> polygon = new ArrayList<Vector2Dd>();
        for ( int i = 0; i < base.size(); i++ ) {
            polygon.add(base.get(i).multiply(factor));
        }
        setBase(polygon);
    }

    private static double cross(double ax, double ay, double bx, double by)
    {
        return ax * by - ay * bx;
    }

    /**
    Exact comparison of two vertices of the base (the `==` of the C++ port:
    unlike `Vector2Dd.equals`, 0.0 and -0.0 are the same coordinate).
    */
    private static boolean sameVertex(Vector2Dd a, Vector2Dd b)
    {
        return a.x == b.x && a.y == b.y;
    }

    /**
    @return true if the closed segments ab and cd share a point
    */
    private static boolean segmentsIntersect(Vector2Dd a, Vector2Dd b,
        Vector2Dd c, Vector2Dd d)
    {
        double d1 = cross(b.x - a.x, b.y - a.y, c.x - a.x, c.y - a.y);
        double d2 = cross(b.x - a.x, b.y - a.y, d.x - a.x, d.y - a.y);
        double d3 = cross(d.x - c.x, d.y - c.y, a.x - c.x, a.y - c.y);
        double d4 = cross(d.x - c.x, d.y - c.y, b.x - c.x, b.y - c.y);

        if ( ((d1 > 0 && d2 < 0) || (d1 < 0 && d2 > 0)) &&
             ((d3 > 0 && d4 < 0) || (d3 < 0 && d4 > 0)) ) {
            return true;
        }
        // Collinear cases: an endpoint on the other segment
        double minX1 = Math.min(a.x, b.x);
        double maxX1 = Math.max(a.x, b.x);
        double minY1 = Math.min(a.y, b.y);
        double maxY1 = Math.max(a.y, b.y);
        double minX2 = Math.min(c.x, d.x);
        double maxX2 = Math.max(c.x, d.x);
        double minY2 = Math.min(c.y, d.y);
        double maxY2 = Math.max(c.y, d.y);
        if ( d1 == 0 && c.x >= minX1 && c.x <= maxX1 && c.y >= minY1 && c.y <= maxY1 ) {
            return true;
        }
        if ( d2 == 0 && d.x >= minX1 && d.x <= maxX1 && d.y >= minY1 && d.y <= maxY1 ) {
            return true;
        }
        if ( d3 == 0 && a.x >= minX2 && a.x <= maxX2 && a.y >= minY2 && a.y <= maxY2 ) {
            return true;
        }
        if ( d4 == 0 && b.x >= minX2 && b.x <= maxX2 && b.y >= minY2 && b.y <= maxY2 ) {
            return true;
        }
        return false;
    }

    private void updateBaseCache()
    {
        int n = base.size();
        validBase = n >= 3;
        minX = minY = Double.POSITIVE_INFINITY;
        maxX = maxY = Double.NEGATIVE_INFINITY;
        for ( int i = 0; i < n; i++ ) {
            Vector2Dd p = base.get(i);
            if ( !Double.isFinite(p.x) || !Double.isFinite(p.y) ) {
                validBase = false;
            }
            minX = Math.min(minX, p.x);
            minY = Math.min(minY, p.y);
            maxX = Math.max(maxX, p.x);
            maxY = Math.max(maxY, p.y);
        }
        if ( !validBase ) {
            return;
        }

        // Non-zero area (setBase made it counterclockwise)
        double twiceArea = 0.0;
        for ( int i = 0; i < n; i++ ) {
            Vector2Dd a = base.get(i);
            Vector2Dd b = base.get((i + 1) % n);
            twiceArea += a.x * b.y - b.x * a.y;
        }
        if ( !(twiceArea > 0.0) ) {
            validBase = false;
            return;
        }

        // Simple polygon: no repeated consecutive vertices, adjacent edges
        // only share their common vertex, other edges share nothing
        for ( int i = 0; i < n && validBase; i++ ) {
            Vector2Dd a = base.get(i);
            Vector2Dd b = base.get((i + 1) % n);
            if ( sameVertex(a, b) ) {
                validBase = false;
                break;
            }
            for ( int j = i + 1; j < n; j++ ) {
                Vector2Dd c = base.get(j);
                Vector2Dd d = base.get((j + 1) % n);
                boolean adjacent = j == i + 1 || (i == 0 && j == n - 1);
                if ( adjacent ) {
                    // Shared vertex: they may not fold back over each other
                    Vector2Dd shared = (j == i + 1) ? b : a;
                    Vector2Dd other1 = (j == i + 1) ? a : b;
                    Vector2Dd other2 = (j == i + 1) ? d : c;
                    double side = cross(other1.x - shared.x, other1.y - shared.y,
                        other2.x - shared.x, other2.y - shared.y);
                    double dot = (other1.x - shared.x) * (other2.x - shared.x) +
                        (other1.y - shared.y) * (other2.y - shared.y);
                    if ( side == 0 && dot > 0 ) {
                        validBase = false;
                        break;
                    }
                }
                else if ( segmentsIntersect(a, b, c, d) ) {
                    validBase = false;
                    break;
                }
            }
        }
    }

    /**
    @return true if the base is a simple polygon of at least three vertices
    and non-zero area, and the height is positive (and everything finite)
    */
    public boolean hasValidParameters()
    {
        return validBase && Double.isFinite(height) && height > 0.0;
    }

    /**
    Triangulates the base by ear clipping.
    @param outTriangles receives three indices of base vertices per
    triangle, counterclockwise seen from +Z
    @return the number of triangles (number of sides - 2), or 0 if the base
    is not valid
    */
    public int triangulateBase(ArrayList<Integer> outTriangles)
    {
        outTriangles.clear();
        if ( !validBase ) {
            return 0;
        }
        ArrayList<Integer> remaining = new ArrayList<Integer>();
        for ( int i = 0; i < base.size(); i++ ) {
            remaining.add(i);
        }
        int triangles = 0;
        while ( remaining.size() > 3 ) {
            int m = remaining.size();
            boolean clipped = false;
            for ( int i = 0; i < m && !clipped; i++ ) {
                Vector2Dd a = base.get(remaining.get((i + m - 1) % m));
                Vector2Dd b = base.get(remaining.get(i));
                Vector2Dd c = base.get(remaining.get((i + 1) % m));
                // Convex corner of a counterclockwise polygon
                if ( !(cross(b.x - a.x, b.y - a.y, c.x - b.x, c.y - b.y) > 0) ) {
                    continue;
                }
                boolean ear = true;
                for ( int j = 0; j < m && ear; j++ ) {
                    if ( j == i || j == (i + m - 1) % m || j == (i + 1) % m ) {
                        continue;
                    }
                    Vector2Dd p = base.get(remaining.get(j));
                    if ( sameVertex(p, a) || sameVertex(p, b) || sameVertex(p, c) ) {
                        continue;
                    }
                    double s1 = cross(b.x - a.x, b.y - a.y, p.x - a.x, p.y - a.y);
                    double s2 = cross(c.x - b.x, c.y - b.y, p.x - b.x, p.y - b.y);
                    double s3 = cross(a.x - c.x, a.y - c.y, p.x - c.x, p.y - c.y);
                    if ( s1 >= 0 && s2 >= 0 && s3 >= 0 ) {
                        ear = false;
                    }
                }
                if ( ear ) {
                    outTriangles.add(remaining.get((i + m - 1) % m));
                    outTriangles.add(remaining.get(i));
                    outTriangles.add(remaining.get((i + 1) % m));
                    remaining.remove(i);
                    triangles++;
                    clipped = true;
                }
            }
            if ( !clipped ) {
                // Only possible with rounding on nearly degenerate bases
                outTriangles.clear();
                return 0;
            }
        }
        outTriangles.add(remaining.get(0));
        outTriangles.add(remaining.get(1));
        outTriangles.add(remaining.get(2));
        return triangles + 1;
    }

    //= Interior of the prism along a line ===============================

    /**
    @return the distance from (x, y) to the closest edge of the base, with
    the index of that edge in `closestEdge[0]`
    */
    private double distanceToBaseBoundary(double x, double y, int[] closestEdge)
    {
        double best = Double.POSITIVE_INFINITY;
        int n = base.size();
        closestEdge[0] = 0;
        for ( int i = 0; i < n; i++ ) {
            Vector2Dd a = base.get(i);
            Vector2Dd b = base.get((i + 1) % n);
            double ex = b.x - a.x;
            double ey = b.y - a.y;
            double lengthSquared = ex * ex + ey * ey;
            double s = lengthSquared > 0 ?
                ((x - a.x) * ex + (y - a.y) * ey) / lengthSquared : 0.0;
            if ( s < 0 ) {
                s = 0;
            }
            else if ( s > 1 ) {
                s = 1;
            }
            double dx = x - (a.x + s * ex);
            double dy = y - (a.y + s * ey);
            double d = Math.sqrt(dx * dx + dy * dy);
            if ( d < best ) {
                best = d;
                closestEdge[0] = i;
            }
        }
        return best;
    }

    /**
    @return 1 if (x, y) is strictly inside the base, 0 if it is on its
    boundary and -1 if it is outside
    */
    private int classifyBasePoint(double x, double y)
    {
        int n = base.size();
        boolean inside = false;
        for ( int i = 0; i < n; i++ ) {
            Vector2Dd a = base.get(i);
            Vector2Dd b = base.get((i + 1) % n);
            double side = cross(b.x - a.x, b.y - a.y, x - a.x, y - a.y);
            if ( side == 0 &&
                 x >= Math.min(a.x, b.x) && x <= Math.max(a.x, b.x) &&
                 y >= Math.min(a.y, b.y) && y <= Math.max(a.y, b.y) ) {
                return 0;
            }
            // Crossing number with the half-open rule on y
            if ( (a.y > y) != (b.y > y) ) {
                double xCross = a.x + (y - a.y) * (b.x - a.x) / (b.y - a.y);
                if ( xCross > x ) {
                    inside = !inside;
                }
            }
        }
        return inside ? 1 : -1;
    }

    /**
    Parts of the line `origin + t * direction` (for every t) inside the
    prism, in increasing order. Empty intervals are discarded, so a line
    that only touches the prism has none.
    @param out receives the intervals
    @return false if the line runs along a face of the prism over an
    interval (it lies on the plane of a cap and crosses the base, or on the
    plane of a wall and crosses the slab), so the interior along it is not
    well defined; `out` then has the intervals as computed with the line
    displaced to one side, which the classic interface still uses
    */
    private boolean computeInteriorIntervals(Vector3Dd origin,
        Vector3Dd direction, ArrayList<Interval> out)
    {
        out.clear();
        boolean welldefined = true;

        //- Slab 0 <= z <= height ----------------------------------------
        Interval slab;
        boolean slabOnCap = false;
        double oz = origin.z();
        double dz = direction.z();
        if ( dz == 0.0 ) {
            if ( oz == 0.0 || oz == height ) {
                slabOnCap = true;
            }
            else if ( !(oz > 0.0 && oz < height) ) {
                return true;
            }
            slab = new Interval(Double.NEGATIVE_INFINITY, SURFACE_NONE,
                Double.POSITIVE_INFINITY, SURFACE_NONE);
        }
        else {
            double tBottom = -oz / dz;
            double tTop = (height - oz) / dz;
            if ( tBottom < tTop ) {
                slab = new Interval(tBottom, SURFACE_BOTTOM, tTop, SURFACE_TOP);
            }
            else {
                slab = new Interval(tTop, SURFACE_TOP, tBottom, SURFACE_BOTTOM);
            }
        }

        //- Parts of the projected line inside the base ------------------
        ArrayList<Interval> planar = new ArrayList<Interval>();
        double ox = origin.x();
        double oy = origin.y();
        double dx = direction.x();
        double dy = direction.y();
        int n = base.size();

        if ( dx == 0.0 && dy == 0.0 ) {
            // Parallel to the walls: everything or nothing
            int c = classifyBasePoint(ox, oy);
            if ( c == 0 ) {
                return false;
            }
            if ( c > 0 ) {
                planar.add(new Interval(Double.NEGATIVE_INFINITY, SURFACE_NONE,
                    Double.POSITIVE_INFINITY, SURFACE_NONE));
            }
        }
        else {
            // Crossings with the edges. A vertex on the line is taken as if it
            // were displaced to the positive side of the line (simulation of
            // simplicity), so the crossings always pair as entering / leaving.
            ArrayList<EdgeCrossing> crossings = new ArrayList<EdgeCrossing>();
            for ( int i = 0; i < n; i++ ) {
                Vector2Dd a = base.get(i);
                Vector2Dd b = base.get((i + 1) % n);
                double sideA = cross(dx, dy, a.x - ox, a.y - oy);
                double sideB = cross(dx, dy, b.x - ox, b.y - oy);
                if ( sideA == 0.0 && sideB == 0.0 ) {
                    // The edge is on the line: the line (which crosses the
                    // slab) runs along its wall
                    welldefined = false;
                    continue;
                }
                if ( (sideA < 0.0) == (sideB < 0.0) ) {
                    continue;
                }
                double ex = b.x - a.x;
                double ey = b.y - a.y;
                double t = cross(a.x - ox, a.y - oy, ex, ey) / cross(dx, dy, ex, ey);
                // Insertion in increasing order of t
                int position = crossings.size();
                crossings.add(new EdgeCrossing(t, i));
                while ( position > 0 && crossings.get(position - 1).lineParameter > t ) {
                    crossings.set(position, crossings.get(position - 1));
                    position--;
                }
                crossings.set(position, new EdgeCrossing(t, i));
            }
            for ( int i = 0; i + 1 < crossings.size(); i += 2 ) {
                double start = crossings.get(i).lineParameter;
                double end = crossings.get(i + 1).lineParameter;
                if ( !(start < end) ) {
                    // The line only touches a vertex
                    continue;
                }
                int startSurface = SURFACE_FIRST_WALL + crossings.get(i).edge;
                int endSurface = SURFACE_FIRST_WALL + crossings.get(i + 1).edge;
                if ( planar.size() > 0 &&
                     planar.get(planar.size() - 1).end == start ) {
                    // The line goes through a reflex vertex: no boundary there
                    planar.get(planar.size() - 1).end = end;
                    planar.get(planar.size() - 1).endSurface = endSurface;
                }
                else {
                    planar.add(new Interval(start, startSurface, end, endSurface));
                }
            }
        }

        //- Slab intersected with the planar intervals -------------------
        for ( int i = 0; i < planar.size(); i++ ) {
            Interval body = new Interval(planar.get(i));
            if ( slab.start > body.start ) {
                body.start = slab.start;
                body.startSurface = slab.startSurface;
            }
            if ( slab.end < body.end ) {
                body.end = slab.end;
                body.endSurface = slab.endSurface;
            }
            if ( body.start < body.end ) {
                if ( slabOnCap ) {
                    // The line lies on the plane of a cap, over the base
                    welldefined = false;
                }
                out.add(body);
            }
        }
        if ( slabOnCap ) {
            // A line on a cap only touches the prism
            out.clear();
        }
        return welldefined;
    }

    /**
    First point of the surface of the prism hit by a ray, after `minT`.
    @param surface output receiving the surface hit in `surface[0]`
    @param overlap output telling in `overlap[0]` if the ray runs along a
    face (see `computeInteriorIntervals`)
    @return the parameter of the hit, or NaN if there is none
    */
    private double firstSurfaceHit(Ray ray, double minT, int[] surface,
        boolean[] overlap)
    {
        ArrayList<Interval> intervals = new ArrayList<Interval>();
        overlap[0] = !computeInteriorIntervals(ray.getOrigin(),
            ray.getDirection(), intervals);
        for ( int i = 0; i < intervals.size(); i++ ) {
            if ( intervals.get(i).start > minT ) {
                surface[0] = intervals.get(i).startSurface;
                return intervals.get(i).start;
            }
            if ( intervals.get(i).end > minT ) {
                surface[0] = intervals.get(i).endSurface;
                return intervals.get(i).end;
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
    hit current Prism
    */
    public Ray doIntersectionFirstHit(Ray inOutRay)
    {
        RayHit hit = new RayHit();
        if ( doIntersectionFirstHit(inOutRay, hit) && hit.getRay() != null ) {
            return new Ray(hit.getRay());
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
        boolean[] overlap = {false};
        double t = firstSurfaceHit(inOutRay, VSDK.EPSILON, surface, overlap);

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
        fillSurfaceData(inRay, inT, surfaceAt(p), outData);
    }

    /**
    @return the surface of the prism nearest to a point of its boundary
    */
    private int surfaceAt(Vector3Dd p)
    {
        int[] edge = {0};
        double dWall = distanceToBaseBoundary(p.x(), p.y(), edge);
        double dBottom = Math.abs(p.z());
        double dTop = Math.abs(p.z() - height);
        int surface = SURFACE_BOTTOM;
        double best = dBottom;

        if ( dTop < best ) {
            best = dTop;
            surface = SURFACE_TOP;
        }
        if ( dWall < best ) {
            surface = SURFACE_FIRST_WALL + edge[0];
        }
        return surface;
    }

    private void fillSurfaceData(Ray ray, double t, int surface,
        RayHit outData)
    {
        outData.beginDetailEvaluation();
        Vector3Dd p = ray.getOrigin().add(ray.getDirection().multiply(t));
        Vector3Dd normal;
        Vector3Dd tangent;
        double u;
        double v;
        double width = maxX - minX;
        double depth = maxY - minY;

        if ( surface == SURFACE_TOP || surface == SURFACE_BOTTOM ) {
            normal = new Vector3Dd(0, 0, surface == SURFACE_TOP ? 1 : -1);
            tangent = new Vector3Dd(1, 0, 0);
            u = width > 0 ? (p.x() - minX) / width : 0;
            v = depth > 0 ? (p.y() - minY) / depth : 0;
        }
        else {
            int n = base.size();
            int edge = surface - SURFACE_FIRST_WALL;
            if ( edge < 0 || edge >= n ) {
                edge = 0;
            }
            Vector2Dd a = base.get(edge);
            Vector2Dd b = base.get((edge + 1) % n);
            double ex = b.x - a.x;
            double ey = b.y - a.y;
            double length = Math.sqrt(ex * ex + ey * ey);
            // Counterclockwise base: the exterior is on the right of each edge
            normal = new Vector3Dd(ey / length, -ex / length, 0);
            tangent = new Vector3Dd(ex / length, ey / length, 0);
            double s = ((p.x() - a.x) * ex + (p.y() - a.y) * ey) /
                (length * length);
            u = (edge + s) / ((double)n);
            v = height > 0 ? p.z() / height : 0;
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
            outData.tangent = tangent;
            outData.markDetailAvailable(RayHit.DETAIL_TANGENT);
        }
        if ( outData.needsTextureCoordinates() ) {
            outData.u = u;
            outData.v = v;
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

        ArrayList<Interval> intervals = new ArrayList<Interval>();
        if ( !computeInteriorIntervals(ray.getOrigin(), ray.getDirection(),
                 intervals) ) {
            return RayCrossingSupport.Indeterminate;
        }
        for ( int i = 0; i < intervals.size(); i++ ) {
            if ( !Double.isFinite(intervals.get(i).start) ||
                 !Double.isFinite(intervals.get(i).end) ) {
                return RayCrossingSupport.Indeterminate;
            }
        }
        for ( int i = 0; i < intervals.size(); i++ ) {
            double entry = intervals.get(i).start;
            double exit = intervals.get(i).end;
            if ( entry >= query.minT && entry <= query.maxT ) {
                RayCrossing crossing = new RayCrossing(entry, this,
                    RayCrossingKind.Entering);
                crossing.surfaceIndex = intervals.get(i).startSurface;
                sink.crossing(crossing);
            }
            if ( exit >= query.minT && exit <= query.maxT ) {
                RayCrossing crossing = new RayCrossing(exit, this,
                    RayCrossingKind.Leaving);
                crossing.surfaceIndex = intervals.get(i).endSurface;
                sink.crossing(crossing);
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
        boolean[] overlap = {false};
        double t = firstSurfaceHit(ray, Math.nextDown(query.minT), surface,
            overlap);

        if ( overlap[0] ) {
            return RayFirstHitStatus.Indeterminate;
        }
        if ( Double.isNaN(t) || t > query.maxT ) {
            return RayFirstHitStatus.Miss;
        }
        if ( boundary != null ) {
            boundary.assign(new RayBoundary(t, this, false, surface[0]));
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
        RayBoundary found = new RayBoundary();
        RayFirstHitStatus status = queryFirstHit(ray,
            new RayCrossingQuery(boundary.t, boundary.t), null, found);
        if ( status != RayFirstHitStatus.Hit ) {
            return status;
        }
        hit.setHitDistance(boundary.t);
        if ( hit.shouldStoreRay() ) {
            hit.setRay(ray.withT(boundary.t));
        }
        int surface = boundary.surfaceIndex >= SURFACE_BOTTOM &&
            boundary.surfaceIndex < SURFACE_FIRST_WALL + base.size() ?
            (int)boundary.surfaceIndex : (int)found.surfaceIndex;
        fillSurfaceData(ray, boundary.t, surface, hit);
        if ( boundary.normalReversed && hit.needsNormal() &&
             (hit.getAvailableDetailMask() & RayHit.DETAIL_NORMAL) != 0 ) {
            hit.normal = hit.normal.multiply(-1);
        }
        return RayFirstHitStatus.Hit;
    }

    /**
    Classifies the ray against the interior of the prism, following the
    contract of `Geometry.classifyRay` (the one of [ROTH1982] CSG): the
    first element of `result.insideSegments` tells if the ray starts (at
    `query.minT`) inside the prism, and each boundary toggles it. A ray
    running along a face is `Indeterminate`.
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
        ArrayList<Interval> intervals = new ArrayList<Interval>();
        if ( !computeInteriorIntervals(ray.getOrigin(), ray.getDirection(),
                 intervals) ) {
            return RaySolidQueryStatus.Indeterminate;
        }
        boolean startsInside = false;

        for ( int i = 0; i < intervals.size(); i++ ) {
            if ( query.minT > intervals.get(i).start &&
                 query.minT <= intervals.get(i).end ) {
                startsInside = true;
            }
        }
        if ( !result.insideSegments.add(startsInside) ) {
            return RaySolidQueryStatus.Indeterminate;
        }
        for ( int i = 0; i < intervals.size(); i++ ) {
            double entry = intervals.get(i).start;
            double exit = intervals.get(i).end;
            if ( entry >= query.minT && entry <= query.maxT ) {
                if ( !result.boundaries.add(new RaySolidBoundary(entry, this,
                         false, intervals.get(i).startSurface)) ||
                     !result.insideSegments.add(true) ) {
                    result.clear();
                    return RaySolidQueryStatus.Indeterminate;
                }
            }
            if ( exit >= query.minT && exit <= query.maxT ) {
                if ( !result.boundaries.add(new RaySolidBoundary(exit, this,
                         false, intervals.get(i).endSurface)) ||
                     !result.insideSegments.add(false) ) {
                    result.clear();
                    return RaySolidQueryStatus.Indeterminate;
                }
            }
        }
        return RaySolidQueryStatus.Supported;
    }

    //= Containment ======================================================

    /**
    Check the general interface contract in superclass method
    Geometry.doContainmentTest. Distances are measured perpendicular to the
    caps and to the walls (in the plane z = 0).
    @param p point to classify, in the space of the prism
    @param distanceTolerance distance to the surface of the prism under
    which the point is classified as `LIMIT`
    @return INSIDE, OUTSIDE or LIMIT constant value
    */
    @Override
    public int doContainmentTest(Vector3Dd p, double distanceTolerance)
    {
        if ( !hasValidParameters() ) {
            return OUTSIDE;
        }
        double z = p.z();
        if ( z < -distanceTolerance || z > height + distanceTolerance ) {
            return OUTSIDE;
        }
        int[] edge = {0};
        double dWall = distanceToBaseBoundary(p.x(), p.y(), edge);
        int c = classifyBasePoint(p.x(), p.y());

        if ( c < 0 ) {
            return dWall <= distanceTolerance ? LIMIT : OUTSIDE;
        }
        if ( c > 0 && dWall > distanceTolerance && z > distanceTolerance &&
             z < height - distanceTolerance ) {
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

    /** Finite bounds: the box of the base, and 0 <= z <= height. */
    @Override
    public void getBounds(GeometryBounds result)
    {
        if ( !hasValidParameters() ) {
            result.setIndeterminate();
            return;
        }
        result.setFinite(new Vector3Dd(minX, minY, 0),
            new Vector3Dd(maxX, maxY, height));
    }

    @Override
    public double[] getMinMax()
    {
        double[] minmax = new double[6];

        minmax[0] = minX;
        minmax[1] = minY;
        minmax[2] = 0;
        minmax[3] = maxX;
        minmax[4] = maxY;
        minmax[5] = height;
        return minmax;
    }

    //= Boundary representation ==========================================

    /**
    The prism is built as in [MANT1988] 12.3.1 (program 12.4): a planar
    lamina of the base polygon (`mvfs`, `smev` along the vertices, `smef`)
    whose face 1 (looking to +Z) is swept `height` units along +Z.
    @return a new boundary representation of the prism
    */
    @Override
    public PolyhedralBoundedSolid exportToPolyhedralBoundedSolid()
    {
        return buildPolyhedralBoundedSolid();
    }

    private PolyhedralBoundedSolid buildPolyhedralBoundedSolid()
    {
        PolyhedralBoundedSolid solid = new PolyhedralBoundedSolid();
        int n = base.size();
        int i;

        PolyhedralBoundedSolidEulerOperators.mvfs(solid,
            new Vector3Dd(base.get(0).x, base.get(0).y, 0.0), 1, 1);
        for ( i = 1; i < n; i++ ) {
            PolyhedralBoundedSolidEulerOperators.smev(solid, 1, i, i + 1,
                new Vector3Dd(base.get(i).x, base.get(i).y, 0.0));
        }
        PolyhedralBoundedSolidEulerOperators.smef(solid, 1, n, 1, 2);
        Matrix4x4d t = new Matrix4x4d();
        t = t.translation(0.0, 0.0, height);
        PolyhedralBoundedSolidEulerConstructions.translationalSweepExtrudeFace(
            solid, solid.findFace(1), t);
        return solid;
    }
}
