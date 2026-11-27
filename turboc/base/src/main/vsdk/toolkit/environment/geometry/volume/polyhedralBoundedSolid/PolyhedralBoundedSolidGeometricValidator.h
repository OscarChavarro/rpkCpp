#ifndef __PBS_GMTRC_VLDTR__
#define __PBS_GMTRC_VLDTR__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"
#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidNumericPolicy.h"
class PolyhedralBoundedSolid;
class _PolyhedralBoundedSolidFace;
class Vector3Dd;

/**
Geometric validation helpers for polyhedral B-Reps, centered on the planar
face model of [MANT1988].10.2.1 and the geometric primitives developed in
chapter [MANT1988].13.

The `msg` parameters play the role of the Java `StringBuilder`: failure
details are appended to them, and they can be null when not needed.
*/
class PolyhedralBoundedSolidGeometricValidator {
public:
    /**
    Checks whether a set of face vertices can define the planar polygon
    required by [MANT1988].10.2.1 and by the face-equation procedure of
    [MANT1988].13.1.
    @param points face points
    @return true when the points are coplanar
    */
    static BOOLEAN validateFacePointsAreCoplanar(ArrayList<Vector3Dd>& points);
    static BOOLEAN validateFacePointsAreCoplanar(ArrayList<Vector3Dd>& points, const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext);

    /**
    Appends the vertex positions of every loop of `face` to `outPoints`.
    @param face face to traverse
    @param outPoints receives the points
    @return false when some loop is empty or not closed (Java returns null)
    */
    static BOOLEAN extractPointsFromFace(_PolyhedralBoundedSolidFace* face, ArrayList<Vector3Dd>& outPoints);

    static BOOLEAN validateFaceIsPlanar(_PolyhedralBoundedSolidFace* face);
    static BOOLEAN validateFaceIsPlanar(_PolyhedralBoundedSolidFace* face, const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext);

    static BOOLEAN validateAllFacesPlanarityAndPlanes(PolyhedralBoundedSolid* solid, String* msg);
    static BOOLEAN validateAllFacesPlanarityAndPlanes(PolyhedralBoundedSolid* solid, const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext, String* msg);

    static BOOLEAN validateConsistentFaceOrientations(PolyhedralBoundedSolid* solid, String* msg);
    static BOOLEAN validateConsistentFaceOrientations(PolyhedralBoundedSolid* solid, const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext, String* msg);

    static BOOLEAN validateLoopsStrict(PolyhedralBoundedSolid* solid, String* msg);
    static BOOLEAN validateLoopsStrict(PolyhedralBoundedSolid* solid, const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext, String* msg);

    static BOOLEAN validateFaceIntersectionsStrict(PolyhedralBoundedSolid* solid, String* msg);
    static BOOLEAN validateFaceIntersectionsStrict(PolyhedralBoundedSolid* solid, const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext, String* msg);

    /**
    Area below which a face is collapsed: VSDK.EPSILON scaled by the square
    of the model scale, so that it equals EPSILON for models of unit size
    and is invariant under a uniform change of units.
    */
    static double collapsedFaceAreaThreshold(const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext);

    /**
    Area of a face: half the length of the Newell vector summed over all
    its loops, so inner loops (holes, oriented the other way) are
    subtracted.
    */
    static double faceArea(_PolyhedralBoundedSolidFace* face);

    /**
    Rejects collapsed laminae. A lamina is a shell made of exactly two
    faces (which then share all their edges, back to back); it has zero
    volume and is a valid result. It is collapsed when its area is below
    collapsedFaceAreaThreshold. Small faces of shells that enclose volume
    (slivers of a tessellation) are not judged by this rule.
    */
    static BOOLEAN validateNoCollapsedLaminae(PolyhedralBoundedSolid* solid, const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext, String* msg);

    /**
    Checks that no two distinct vertices of `solid` are geometrically
    coincident (distance under `context.bigEpsilon()`).
    @param solid solid to inspect
    @param context tolerance context
    @param msg collects one line per coincident pair found
    @return true when no coincident vertices are found
    */
    static BOOLEAN validateNoCoincidentVertices(PolyhedralBoundedSolid* solid, const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& context, String* msg);

    /**
    Checks that face and vertex ids are unique and covered by the stored
    maxima of the solid.
    @param solid solid to inspect
    @param msg collects one line per violation found
    @return true when ids are consistent
    */
    static BOOLEAN validateUniqueFaceAndVertexIds(PolyhedralBoundedSolid* solid, String* msg);

private:
    static double solidDiagonal(PolyhedralBoundedSolid* solid);
};

#endif
