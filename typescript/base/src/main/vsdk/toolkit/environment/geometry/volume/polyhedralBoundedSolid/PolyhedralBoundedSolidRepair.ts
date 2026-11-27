//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

import type { PolyhedralBoundedSolid } from "./PolyhedralBoundedSolid.js";
import type { _PolyhedralBoundedSolidHalfEdge } from "./nodes/_PolyhedralBoundedSolidHalfEdge.js";
import type { _PolyhedralBoundedSolidLoop } from "./nodes/_PolyhedralBoundedSolidLoop.js";

/**
Explicit repairs of a solid. Validation
({@link PolyhedralBoundedSolidValidationEngine}) only reads: it never calls
anything here. A caller that wants a structure repaired asks for it and
gets a report of what changed.

<p>Before plan-stage11 Phase C the repair below ran inside every
validation, so validating a solid could change it.</p>
*/
export class PolyhedralBoundedSolidRepair {
    private constructor() {}

    /**
    Rebuilds the vertex-to-emanating-half-edge links required by the vertex
    node of [MANT1988].10.2.1 and used throughout the Euler operators of
    chapter [MANT1988].11: every vertex is linked to the last half-edge
    that starts at it, in face, loop and half-edge order, and vertices that
    no half-edge starts at are removed from the solid.
    @return what was wrong before the rebuild
    */
    public static remakeEmanatingHalfEdgeReferences(
        solid: PolyhedralBoundedSolid,
    ): PolyhedralBoundedSolidRepair.Report {
        let relinked = 0;
        let removed = 0;
        let i: number;
        let j: number;

        for (i = 0; i < solid.getVerticesList().size(); i++) {
            const v = solid.getVerticesList().get(i)!;
            const he = v.emanatingHalfEdge;
            if (
                he === null ||
                he.startingVertex !== v ||
                he.parentLoop === null ||
                !he.parentLoop.halfEdgesList.contains(he)
            ) {
                relinked++;
            }
            v.emanatingHalfEdge = null;
        }

        for (i = 0; i < solid.getPolygonsList().size(); i++) {
            const face = solid.getPolygonsList().get(i)!;
            for (j = 0; j < face.boundariesList.size(); j++) {
                let loop: _PolyhedralBoundedSolidLoop;
                let he: _PolyhedralBoundedSolidHalfEdge | null;
                let heStart: _PolyhedralBoundedSolidHalfEdge;

                loop = face.boundariesList.get(j)!;
                he = loop.boundaryStartHalfEdge;
                if (he === null) {
                    continue;
                }
                heStart = he;
                do {
                    he!.startingVertex.emanatingHalfEdge = he;
                    he = he!.next();
                    if (he === null) {
                        break;
                    }
                } while (he !== heStart);
            }
        }

        for (i = 0; i < solid.getVerticesList().size(); i++) {
            if (solid.getVerticesList().get(i)!.emanatingHalfEdge === null) {
                solid.getVerticesList().remove(i);
                relinked--;
                removed++;
                i--;
            }
        }
        return new PolyhedralBoundedSolidRepair.Report(relinked, removed);
    }
}

export namespace PolyhedralBoundedSolidRepair {
    /** What a repair changed. */
    export class Report {
        private readonly relinkedVertices: number;
        private readonly removedVertices: number;

        /** @internal Built by PolyhedralBoundedSolidRepair only. */
        public constructor(relinkedVertices: number, removedVertices: number) {
            this.relinkedVertices = relinkedVertices;
            this.removedVertices = removedVertices;
        }

        /**
        Vertices whose emanating half-edge was missing or wrong (not
        starting at the vertex, or not a member of its loop) and was set.
        */
        public getRelinkedVertices(): number {
            return this.relinkedVertices;
        }

        /** Vertices removed because no half-edge starts at them. */
        public getRemovedVertices(): number {
            return this.removedVertices;
        }

        /** True when the repair found nothing to fix. */
        public isClean(): boolean {
            return this.relinkedVertices === 0 && this.removedVertices === 0;
        }

        public toString(): string {
            return (
                "Report{relinkedVertices=" + this.relinkedVertices + ", removedVertices=" + this.removedVertices + "}"
            );
        }
    }
}
