#ifndef __PRISM__
#define __PRISM__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector2Dd.h"
#include "vsdk/toolkit/environment/geometry/volume/Solid.h"

class PolyhedralBoundedSolid;
class Ray;
class RayHit;

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

The surfaces of the prism are numbered (`RayBoundary::surfaceIndex`):
`SURFACE_BOTTOM` (z = 0), `SURFACE_TOP` (z = height) and, for each edge i
of the base (from vertex i to vertex i+1), the wall `SURFACE_FIRST_WALL` + i.

For generic editors (as the scene editor) it exposes the number of sides of
the base, its circumradius (the largest distance from the Z axis to a vertex
of the base) and its height. Changing the number of sides replaces the base
by a regular polygon with the same circumradius; changing the circumradius
scales the base about the Z axis.
*/
class Prism : public Solid {
public:
    enum {
        SURFACE_NONE = -1,
        SURFACE_BOTTOM = 0,
        SURFACE_TOP = 1,
        SURFACE_FIRST_WALL = 2
    };

    static const Class CLASS;
    virtual const Class *getClass() const;

private:
    ArrayList<Vector2Dd> base;
    double height;
    /// Cached from the base: true when it is a simple polygon of non-zero area
    BOOLEAN validBase;
    double minX;
    double minY;
    double maxX;
    double maxY;

#ifdef __TURBOC__
// C++ port note: public for Turbo C++ 3.0, as in Tube and
// _PolyhedralBoundedSolidSetNonIntersectingClassifier: the ArrayList
// instances of the nested classes must reach them
public:
#endif
    /**
    Interval [start, end] of the parameter of a line, whose ends are on the
    given surfaces of the prism (`SURFACE_NONE` for an unbounded end).
    */
    class Interval {
    public:
        double start;
        double end;
        int startSurface;
        int endSurface;

        Interval() : start(0), end(0), startSurface(SURFACE_NONE),
            endSurface(SURFACE_NONE) {}
        Interval(double start, int startSurface, double end, int endSurface)
            : start(start), end(end), startSurface(startSurface),
            endSurface(endSurface) {}
    };

    /** A crossing of a line with an edge of the base, on the plane z = 0. */
    class EdgeCrossing {
    public:
        double lineParameter;
        int edge;

        EdgeCrossing() : lineParameter(0), edge(0) {}
        EdgeCrossing(double t, int edge) : lineParameter(t), edge(edge) {}
    };
#ifdef __TURBOC__
private:
#endif

    void addControlSpecifications();
    void updateBaseCache();
    static BOOLEAN segmentsIntersect(const Vector2Dd& a, const Vector2Dd& b,
        const Vector2Dd& c, const Vector2Dd& d);
    static double cross(double ax, double ay, double bx, double by);

    /**
    @return the distance from (x, y) to the closest edge of the base, with
    the index of that edge in `closestEdge`
    */
    double distanceToBaseBoundary(double x, double y, int& closestEdge) const;

    /**
    @return 1 if (x, y) is strictly inside the base, 0 if it is on its
    boundary and -1 if it is outside
    */
    int classifyBasePoint(double x, double y) const;

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
    BOOLEAN computeInteriorIntervals(const Vector3Dd& origin,
        const Vector3Dd& direction, ArrayList<Interval>& out) const;

    /**
    First point of the surface of the prism hit by a ray, after `minT`.
    @param surface output receiving the surface hit
    @param overlap output telling if the ray runs along a face (see
    `computeInteriorIntervals`)
    @return the parameter of the hit, or NaN if there is none
    */
    double firstSurfaceHit(const Ray& ray, double minT, int& surface,
        BOOLEAN& overlap) const;

    /**
    @return the surface of the prism nearest to a point of its boundary
    */
    int surfaceAt(const Vector3Dd& p) const;

    void fillSurfaceData(const Ray& ray, double t, int surface,
        RayHit* outData) const;

    /**
    C++ port style of `Tube::validateQuery`.
    @return true if the query and the ray can not be processed, with the
    reason in `failure`
    */
    BOOLEAN validateQuery(const Ray& ray, const RayCrossingQuery& query,
        RayFirstHitStatus::Value& failure) const;

    PolyhedralBoundedSolid* buildPolyhedralBoundedSolid();

public:
    /**
    Regular prism: the base is a regular polygon of `numberOfSides` vertices
    at distance `circumradius` from the Z axis, the first one on +X.
    */
    Prism(int numberOfSides, double circumradius, double height);

    /**
    @param base vertices of a simple polygon on the plane z = 0, in either
    orientation (it is stored counterclockwise)
    @param height length of the sweep along +Z
    */
    Prism(const ArrayList<Vector2Dd>& base, double height);
    virtual ~Prism() {}

    virtual Geometry* copyGeometry() const;

    //= Parameters ======================================================
    const ArrayList<Vector2Dd>& getBase() const;
    void setBase(const ArrayList<Vector2Dd>& value);
    double getHeight() const;
    void setHeight(double value);
    int getNumberOfSides() const;
    void setNumberOfSides(int value);
    double getCircumradius() const;
    void setCircumradius(double value);

    /**
    @return true if the base is a simple polygon of at least three vertices
    and non-zero area, and the height is positive (and everything finite)
    */
    BOOLEAN hasValidParameters() const;

    /**
    Triangulates the base by ear clipping.
    @param outTriangles receives three indices of base vertices per
    triangle, counterclockwise seen from +Z
    @return the number of triangles (number of sides - 2), or 0 if the base
    is not valid
    */
    int triangulateBase(ArrayList<int>& outTriangles) const;

    //= Classic ray intersection interface ==============================
    Ray* doIntersectionFirstHit(const Ray& inOutRay);
    virtual BOOLEAN doIntersectionFirstHit(const Ray& inRay, RayHit* outHit);
    virtual void doExtraInformation(const Ray& inRay, double inT,
        RayHit* outData);

    //= Ray crossing and classification interface ======================
    virtual RayCrossingSupport::Value emitRayCrossings(const Ray& ray,
        const RayCrossingQuery& query, RayCrossingSink& sink) const;
    virtual RayFirstHitStatus::Value queryFirstHit(const Ray& ray,
        const RayCrossingQuery& query, RayHit* result,
        RayBoundary* boundary = NULL) const;
    virtual RayFirstHitStatus::Value querySurfaceDetail(const Ray& ray,
        const RayBoundary& boundary, RayHit* hit) const;

    /**
    Classifies the ray against the interior of the prism, following the
    contract of `Geometry.classifyRay` (the one of [ROTH1982] CSG): the
    first element of `result.insideSegments` tells if the ray starts (at
    `query.minT`) inside the prism, and each boundary toggles it. A ray
    running along a face is `Indeterminate`.
    */
    virtual RaySolidQueryStatus::Value classifyRay(const Ray& ray,
        const RayCrossingQuery& query,
        RaySolidClassification& result) const;

    //= Containment and bounds ==========================================
    /**
    Check the general interface contract in superclass method
    Geometry.doContainmentTest. Distances are measured perpendicular to the
    caps and to the walls (in the plane z = 0).
    */
    virtual int doContainmentTest(const Vector3Dd& p, double distanceTolerance);
    virtual GeometryContainment::Value classifyPoint(const Vector3Dd& p,
        double distanceTolerance) const;

    /** Finite bounds: the box of the base, and 0 <= z <= height. */
    virtual void getBounds(GeometryBounds& result) const;
    virtual double* getMinMax();

    //= Boundary representation =========================================
    /**
    The prism is built as in [MANT1988] 12.3.1 (program 12.4): a planar
    lamina of the base polygon (`mvfs`, `smev` along the vertices, `smef`)
    whose face 1 (looking to +Z) is swept `height` units along +Z.
    @return a new boundary representation of the prism, owned by the caller
    */
    virtual PolyhedralBoundedSolid* exportToPolyhedralBoundedSolid();
};

#endif
