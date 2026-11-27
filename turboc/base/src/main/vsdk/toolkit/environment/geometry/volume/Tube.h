//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#ifndef __TUBE__
#define __TUBE__

#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/volume/Solid.h"
class PolyhedralBoundedSolid;
class Vector3Dd;
class Ray;
class RayHit;

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
class Tube : public Solid {
private:
    enum {
        DEFAULT_CIRCUMFERENCE_DIVISIONS = 36,
        MIN_CIRCUMFERENCE_DIVISIONS = 3
    };

#ifdef __TURBOC__
// Turbo C++ 3.0 does not let the nested Interval read private members
public:
#endif
    /// Surfaces of the tube, used to tag the ends of the interior intervals
    enum {
        SURFACE_NONE = 0,
        SURFACE_BOTTOM = 1,
        SURFACE_TOP = 2,
        SURFACE_EXTERNAL = 3,
        SURFACE_INTERNAL = 4
    };
#ifdef __TURBOC__
private:
#endif

    double internalRadius;
    double externalRadius;
    double height;

    /**
    @return true if the parameters describe a tube this class can work with
    */
    BOOLEAN hasValidParameters() const;

    //= Interior of the tube along a ray =================================

    /**
    Interval [start, end] of the parameter of a line, whose ends are on the
    given surfaces of the tube (`SURFACE_NONE` for an unbounded end).
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

    /**
    C++ port note: Java returns null when there is no interval; here the
    result is false and `out` is not changed.
    @return true and the interval of the line `origin + t * direction`
    inside the infinite cylinder of the given radius around Z, or false if
    the line does not cross it (or only touches it)
    */
    static BOOLEAN cylinderInterval(const Vector3Dd& origin,
        const Vector3Dd& direction, double radius, int surface,
        Interval& out);

    /**
    @return true and the interval of the line inside the slab
    0 <= z <= height, or false if it does not cross it
    */
    BOOLEAN slabInterval(const Vector3Dd& origin, const Vector3Dd& direction,
        Interval& out) const;

    /**
    Calculates the parts of the line `origin + t * direction` (for every t)
    inside the tube, in increasing order. Empty intervals are discarded, so a
    line tangent to the tube has none.
    @param out receives up to two intervals
    @return number of intervals
    */
    int computeInteriorIntervals(const Vector3Dd& origin,
        const Vector3Dd& direction, Interval out[2]) const;

    /**
    @return the parameter where the line crosses the washer (the plane z = 0
    between both radii), or NaN if it does not
    */
    double washerHit(const Vector3Dd& origin, const Vector3Dd& direction) const;

    /**
    First point of the surface of the tube hit by a ray, after `minT`.
    @param surface one-element output holder receiving the surface hit
    @return the parameter of the hit, or NaN if there is none
    */
    double firstSurfaceHit(const Ray& ray, double minT, int surface[1]) const;

    /**
    @return the surface of the tube nearest to a point of its boundary
    */
    int surfaceAt(const Vector3Dd& p, const Vector3Dd& rayDirection) const;

    void fillSurfaceData(const Ray& ray, double t, int surface,
        RayHit* outData) const;

    /**
    C++ port note: Java returns null when the query and the ray can be
    processed; here `RayFirstHitStatus::Hit` means that.
    @return Hit if the query and the ray can be processed, or the status
    explaining why they can not
    */
    RayFirstHitStatus::Value validateQuery(const Ray& ray,
        const RayCrossingQuery& query) const;

    /**
    The tube is built as a translational sweep ([MANT1988] 12.3.1, program
    12.4) of a washer lamina (see `createWasherLamina`): its face 1, with
    both loops, is extruded `height` units along +Z. A washer is just the
    lamina.
    */
    PolyhedralBoundedSolid* buildPolyhedralBoundedSolid(int nsides);

    /**
    @return the id of the face among `candidateFaceIds` that has both
    vertices on one of its loops, or 0 if there is none
    */
    static int findFaceWithVertices(PolyhedralBoundedSolid* solid,
        const ArrayList<int>& candidateFaceIds, int vertexId1,
        int vertexId2);

    /**
    Splits both faces of a washer lamina into quadrilaterals (see
    `createWasherLamina`).
    */
    static void splitWasherIntoQuadrilaterals(PolyhedralBoundedSolid* solid,
        int n);

public:
    static const Class CLASS;
    virtual const Class *getClass() const;

    Tube(double internalRadius, double externalRadius, double height);
    virtual ~Tube() {}

    double getInternalRadius() const;
    double getExternalRadius() const;
    double getHeight() const;
    void setInternalRadius(double value);
    void setExternalRadius(double value);
    void setHeight(double value);

    /**
    @return true if this tube has no height, so it is a washer (a planar
    lamina with a hole)
    */
    BOOLEAN isWasher() const;

    virtual Geometry* copyGeometry() const;

    //= Classic ray intersection interface ===============================

    /**
    Check the general interface contract in superclass method
    Geometry.doIntersectionFirstHit.
    @param inOutRay
    @return a new ray with the distance to the hit (owned by the caller), or
    null if the ray does not hit current Tube
    */
    Ray* doIntersectionFirstHit(const Ray& inOutRay);
    virtual BOOLEAN doIntersectionFirstHit(const Ray& inOutRay, RayHit* outHit);

    /**
    Check the general interface contract in superclass method
    Geometry.doExtraInformation.
    */
    virtual void doExtraInformation(const Ray& inRay, double inT, RayHit* outData);

    //= Ray crossing and classification interface ========================

    virtual RayCrossingSupport::Value emitRayCrossings(const Ray& ray,
        const RayCrossingQuery& query, RayCrossingSink& sink) const;
    virtual RayFirstHitStatus::Value queryFirstHit(const Ray& ray,
        const RayCrossingQuery& query, RayHit* result,
        RayBoundary* boundary = NULL) const;
    virtual RayFirstHitStatus::Value querySurfaceDetail(const Ray& ray,
        const RayBoundary& boundary, RayHit* hit) const;

    /**
    Classifies the ray against the interior of the tube, following the
    contract of `Geometry.classifyRay`: the first element of
    `result.insideSegments` tells if the ray starts (at `query.minT`) inside
    the tube, and each boundary toggles it. A washer has no interior.
    */
    virtual RaySolidQueryStatus::Value classifyRay(const Ray& ray,
        const RayCrossingQuery& query, RaySolidClassification& result) const;

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
    virtual int doContainmentTest(const Vector3Dd& p, double distanceTolerance);
    virtual GeometryContainment::Value classifyPoint(const Vector3Dd& p,
        double distanceTolerance) const;

    //= Bounds ===========================================================

    /** Finite bounds: |x|,|y| <= externalRadius and 0 <= z <= height. */
    virtual void getBounds(GeometryBounds& result) const;
    virtual double* getMinMax();

    //= Boundary representation ==========================================

    /**
    @return a new boundary representation of the tube with the default
    divisions, owned by the caller
    */
    virtual PolyhedralBoundedSolid* exportToPolyhedralBoundedSolid();

    /**
    @param circumferenceDivisions number of sides of the polygons
    approximating the circles (at least 3)
    @return the boundary representation of the tube, owned by the caller
    */
    PolyhedralBoundedSolid* exportToPolyhedralBoundedSolid(
        int circumferenceDivisions);

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
    @param withQuadrilaterals false (the default) for two faces with a hole,
    true to split them in quadrilaterals (triangles for a disk)
    @return the lamina (owned by the caller); face 1 is the one facing +Z
    (it is the one to sweep)
    */
    static PolyhedralBoundedSolid* createWasherLamina(double ri, double re,
        double h, int n, BOOLEAN withQuadrilaterals = FALSE);

    /**
    Splits face `faceId` of `solid`, a single loop polygon, into a fan of
    triangles towards a new vertex at `center` (`smev` from the first vertex
    of the loop, then `mef` around it). The face keeps one of the triangles
    and the others get new face ids, in the order of the loop.
    @return the id of the new center vertex, or 0 if the face does not
    exist or is not a polygon of at least three vertices
    */
    static int splitFaceIntoFan(PolyhedralBoundedSolid* solid, int faceId,
        const Vector3Dd& center);
};

#endif
