//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

package vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid;

import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidFace;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidHalfEdge;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidLoop;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidVertex;

/**
Explicit repairs of a solid. Validation
({@link PolyhedralBoundedSolidValidationEngine}) only reads: it never calls
anything here. A caller that wants a structure repaired asks for it and
gets a report of what changed.

<p>Before plan-stage11 Phase C the repair below ran inside every
validation, so validating a solid could change it.</p>
*/
public final class PolyhedralBoundedSolidRepair
{
    /** What a repair changed. */
    public static final class Report
    {
        private final int relinkedVertices;
        private final int removedVertices;

        private Report(int relinkedVertices, int removedVertices)
        {
            this.relinkedVertices = relinkedVertices;
            this.removedVertices = removedVertices;
        }

        /**
        Vertices whose emanating half-edge was missing or wrong (not
        starting at the vertex, or not a member of its loop) and was set.
        */
        public int getRelinkedVertices()
        {
            return relinkedVertices;
        }

        /** Vertices removed because no half-edge starts at them. */
        public int getRemovedVertices()
        {
            return removedVertices;
        }

        /** True when the repair found nothing to fix. */
        public boolean isClean()
        {
            return relinkedVertices == 0 && removedVertices == 0;
        }

        @Override
        public String toString()
        {
            return "Report{relinkedVertices=" + relinkedVertices +
                ", removedVertices=" + removedVertices + "}";
        }
    }

    private PolyhedralBoundedSolidRepair()
    {
    }

    /**
    Rebuilds the vertex-to-emanating-half-edge links required by the vertex
    node of [MANT1988].10.2.1 and used throughout the Euler operators of
    chapter [MANT1988].11: every vertex is linked to the last half-edge
    that starts at it, in face, loop and half-edge order, and vertices that
    no half-edge starts at are removed from the solid.
    @return what was wrong before the rebuild
    */
    public static Report remakeEmanatingHalfEdgeReferences(
        PolyhedralBoundedSolid solid)
    {
        int relinked = 0;
        int removed = 0;
        int i;
        int j;

        for ( i = 0; i < solid.getVerticesList().size(); i++ ) {
            _PolyhedralBoundedSolidVertex v = solid.getVerticesList().get(i);
            _PolyhedralBoundedSolidHalfEdge he = v.emanatingHalfEdge;
            if ( he == null || he.startingVertex != v ||
                 he.parentLoop == null ||
                 !he.parentLoop.halfEdgesList.contains(he) ) {
                relinked++;
            }
            v.emanatingHalfEdge = null;
        }

        for ( i = 0; i < solid.getPolygonsList().size(); i++ ) {
            _PolyhedralBoundedSolidFace face = solid.getPolygonsList().get(i);
            for ( j = 0; j < face.boundariesList.size(); j++ ) {
                _PolyhedralBoundedSolidLoop loop;
                _PolyhedralBoundedSolidHalfEdge he;
                _PolyhedralBoundedSolidHalfEdge heStart;

                loop = face.boundariesList.get(j);
                he = loop.boundaryStartHalfEdge;
                if ( he == null ) {
                    continue;
                }
                heStart = he;
                do {
                    he.startingVertex.emanatingHalfEdge = he;
                    he = he.next();
                    if ( he == null ) {
                        break;
                    }
                } while( he != heStart );
            }
        }

        for ( i = 0; i < solid.getVerticesList().size(); i++ ) {
            if ( solid.getVerticesList().get(i).emanatingHalfEdge == null ) {
                solid.getVerticesList().remove(i);
                relinked--;
                removed++;
                i--;
            }
        }
        return new Report(relinked, removed);
    }
}
