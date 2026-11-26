//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

#ifndef __POLYHEDRAL_BOUNDED_SOLID_REPAIR__
#define __POLYHEDRAL_BOUNDED_SOLID_REPAIR__

#include "java/lang/String.h"
class PolyhedralBoundedSolid;

/**
Explicit repairs of a solid. Validation
(`PolyhedralBoundedSolidValidationEngine`) only reads: it never calls
anything here. A caller that wants a structure repaired asks for it and
gets a report of what changed.

Before plan-stage11 Phase C the repair below ran inside every
validation, so validating a solid could change it.
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
        bool isClean() const;

        java::String toString() const;
    };

    /**
    Rebuilds the vertex-to-emanating-half-edge links required by the vertex
    node of [MANT1988].10.2.1 and used throughout the Euler operators of
    chapter [MANT1988].11: every vertex is linked to the last half-edge
    that starts at it, in face, loop and half-edge order, and vertices that
    no half-edge starts at are removed from the solid.

    C++ port note: as in Java, removed vertices are only unlisted from the
    solid; they are not deleted here.
    @return what was wrong before the rebuild
    */
    static Report remakeEmanatingHalfEdgeReferences(
        PolyhedralBoundedSolid* solid);

private:
    PolyhedralBoundedSolidRepair();
};

#endif
