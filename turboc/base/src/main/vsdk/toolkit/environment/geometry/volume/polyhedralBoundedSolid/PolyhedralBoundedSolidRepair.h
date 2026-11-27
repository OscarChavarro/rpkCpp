//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#ifndef __PBS_REPAIR__
#define __PBS_REPAIR__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/lang/String.h"
class PolyhedralBoundedSolid;

/**
Explicit repairs of a solid. Validation
({@link PolyhedralBoundedSolidValidationEngine}) only reads: it never calls
anything here. A caller that wants a structure repaired asks for it and
gets a report of what changed.

<p>Before plan-stage11 Phase C the repair below ran inside every
validation, so validating a solid could change it.</p>
*/
class PolyhedralBoundedSolidRepair {
public:
    /** What a repair changed. */
    class Report {
    private:
        int relinkedVertices;
        int removedVertices;

    public:
        Report(int relinkedVertices, int removedVertices);

        /**
        Vertices whose emanating half-edge was missing or wrong (not
        starting at the vertex, or not a member of its loop) and was set.
        */
        int getRelinkedVertices() const;

        /** Vertices removed because no half-edge starts at them. */
        int getRemovedVertices() const;

        /** True when the repair found nothing to fix. */
        BOOLEAN isClean() const;

        String toString() const;
    };

    static Report remakeEmanatingHalfEdgeReferences(
        PolyhedralBoundedSolid* solid);

private:
    PolyhedralBoundedSolidRepair();
};

#endif
