#ifndef __POLYHEDRAL_BOUNDED_SOLID_BOOLEAN_TOPOLOGY_PREDICATES__
#define __POLYHEDRAL_BOUNDED_SOLID_BOOLEAN_TOPOLOGY_PREDICATES__

#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidNumericPolicy.h"
class InfinitePlane;
class _PolyhedralBoundedSolidHalfEdge;
class _PolyhedralBoundedSolidLoop;

/**
Utility predicates for Boolean topology support on polyhedral bounded solids.
*/
class _PolyhedralBoundedSolidBooleanTopologyPredicates {
public:
    /**
    @param a first plane, can be null
    @param b second plane, can be null
    @param tolerance comparison tolerance over normalized plane equations
    @return true when both planes are the same, in any orientation
    */
    static bool planesCoincidentIgnoringOrientation(
        const InfinitePlane* a,
        const InfinitePlane* b,
        double tolerance);

    /**
    @param startA starting half-edge over first loop
    @param startB starting half-edge over second loop
    @param reverse true to traverse second loop backwards
    @param numericContext numeric tolerance context
    @return true when both loops visit coincident vertex positions
    */
    static bool loopsCoincidentFrom(
        _PolyhedralBoundedSolidHalfEdge* startA,
        _PolyhedralBoundedSolidHalfEdge* startB,
        bool reverse,
        const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext);

    /**
    @param a first loop
    @param b second loop
    @param numericContext numeric tolerance context
    @return true when both loops have the same vertex positions sequence, in
    any orientation and from any starting vertex
    */
    static bool loopsCoincident(
        _PolyhedralBoundedSolidLoop* a,
        _PolyhedralBoundedSolidLoop* b,
        const PolyhedralBoundedSolidNumericPolicy::ToleranceContext& numericContext);

private:
    _PolyhedralBoundedSolidBooleanTopologyPredicates();
};

#endif
