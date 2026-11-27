#ifndef __PBS_TOPOLOGICAL_VALIDATOR__
#define __PBS_TOPOLOGICAL_VALIDATOR__

#include "vsdk/toolkit/common/VSDK.h"
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
    static BOOLEAN validateTopologicalIntegrity(PolyhedralBoundedSolid* solid);

    /**
    Back-pointer invariants of the half-edge structure ([MANT1988] Sec.
    10.2; Fable5.2Analysis.md Stage B step 3):
    - every loop's `boundaryStartHalfEdge` is a member of the loop, and every
      member half-edge names that loop as `parentLoop`;
    - every vertex has a non-null emanating half-edge that starts at the
      vertex and is a member of its loop (for the lone vertex of an empty
      loop, that is the loop's edge-less half-edge).
    Validation never repairs them: see
    {@link PolyhedralBoundedSolidRepair#remakeEmanatingHalfEdgeReferences}.
    @return true when all hold; violations are appended to `msg`.
    */
    static BOOLEAN validateBackPointers(PolyhedralBoundedSolid* solid,
        String* msg);

private:
    _PolyhedralBoundedSolidTopologicalValidator();
};

#endif
