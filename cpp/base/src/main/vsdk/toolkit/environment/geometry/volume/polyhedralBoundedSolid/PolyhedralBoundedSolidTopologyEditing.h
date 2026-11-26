#ifndef __POLYHEDRAL_BOUNDED_SOLID_TOPOLOGY_EDITING__
#define __POLYHEDRAL_BOUNDED_SOLID_TOPOLOGY_EDITING__

#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidNumericPolicy.h"
class PolyhedralBoundedSolid;
class _PolyhedralBoundedSolidFace;

/**
Contains topology-editing operations for `PolyhedralBoundedSolid` that are
built on Euler operators.
*/
class PolyhedralBoundedSolidTopologyEditing {
public:
    /**
    After section [MANT1988].12.4.2 and program [MANT1988].12.9.
    @param solid target solid instance
    @param faceId face id to glue
    */
    static void loopGlue(PolyhedralBoundedSolid* solid, int faceId);

    /**
    Glues two coincident loops from a face by applying Euler operators.
    @param solid target solid instance
    @param face face containing at least two loops to glue
    */
    static void loopGlue(PolyhedralBoundedSolid* solid, _PolyhedralBoundedSolidFace* face);

    /**
    Modifies ids for current solid's vertices, edges, faces and half-edges to
    make them consecutive from 1.
    @param solid target solid instance
    */
    static void compactIds(PolyhedralBoundedSolid* solid);

    /**
    Removes all "inessential" edges of current solid (i.e. edges that
    separates two coplanar faces, or that occurs just in a single face).
    This is an answer to problem [MANT1988].15.2, needed by the maximal
    faces requirement of section [MANT1988].15.5.
    @param solid target solid instance
    */
    static void maximizeFaces(PolyhedralBoundedSolid* solid);

    /**
    Removes edges whose two endpoints are geometrically coincident, merging
    the two vertices into one via `lkev`, as the pre-processing step of
    [MANT1988].15 before `setOpGenerate`.
    @param solid target solid instance
    @param context tolerance context used to detect coincidence
    @return number of edges collapsed
    */
    static int weldCoincidentVertices(PolyhedralBoundedSolid* solid,
        const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& context);

private:
    PolyhedralBoundedSolidTopologyEditing();
};

#endif
