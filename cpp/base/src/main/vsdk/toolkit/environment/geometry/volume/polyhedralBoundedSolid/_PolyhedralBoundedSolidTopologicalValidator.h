#ifndef __POLYHEDRAL_BOUNDED_SOLID_TOPOLOGICAL_VALIDATOR__
#define __POLYHEDRAL_BOUNDED_SOLID_TOPOLOGICAL_VALIDATOR__

#include "java/lang/String.h"
class PolyhedralBoundedSolid;

/**
Topological validation helpers for the half-edge representation described in
[MANT1988].10.2.1 and [MANT1988].10.2.2.
*/
class _PolyhedralBoundedSolidTopologicalValidator {
public:
    /**
    Checks the fundamental half-edge consistency expected from the graph and
    identification structure described in [MANT1988].10.2.1 and
    [MANT1988].10.2.2.
    @param solid solid to check
    @return true when every edge is used exactly by two half-edges of closed
    loops
    */
    static bool validateTopologicalIntegrity(PolyhedralBoundedSolid* solid);

    /**
    Back-pointer invariants of the half-edge structure ([MANT1988] Sec.
    10.2): loop start half-edges and parent loops, and the emanating
    half-edge of every vertex. Validation never repairs them: see
    `PolyhedralBoundedSolidRepair::remakeEmanatingHalfEdgeReferences`.
    @param solid solid to check
    @param msg receives one line per violation
    @return true when all hold
    */
    static bool validateBackPointers(PolyhedralBoundedSolid* solid,
        java::String* msg);

private:
    _PolyhedralBoundedSolidTopologicalValidator();
};

#endif
