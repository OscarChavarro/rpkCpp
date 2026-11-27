//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

import type { StringBuilder } from "../../../../../../java/lang/StringBuilder.js";
import { VSDK } from "../../../../common/VSDK.js";
import { Logger } from "../../../../common/logging/Logger.js";
import type { PolyhedralBoundedSolid } from "./PolyhedralBoundedSolid.js";
import type { _PolyhedralBoundedSolidEdge } from "./nodes/_PolyhedralBoundedSolidEdge.js";
import type { _PolyhedralBoundedSolidFace } from "./nodes/_PolyhedralBoundedSolidFace.js";
import type { _PolyhedralBoundedSolidHalfEdge } from "./nodes/_PolyhedralBoundedSolidHalfEdge.js";
import type { _PolyhedralBoundedSolidLoop } from "./nodes/_PolyhedralBoundedSolidLoop.js";
import type { _PolyhedralBoundedSolidVertex } from "./nodes/_PolyhedralBoundedSolidVertex.js";

/**
Topological validation helpers for the half-edge representation described in
[MANT1988].10.2.1 and [MANT1988].10.2.2.
*/
export class _PolyhedralBoundedSolidTopologicalValidator {
    private constructor() {}

    /**
    Checks the fundamental half-edge consistency expected from the graph and
    identification structure described in [MANT1988].10.2.1 and
    [MANT1988].10.2.2.
    */
    public static validateTopologicalIntegrity(solid: PolyhedralBoundedSolid): boolean {
        let i: number;
        let j: number;
        let k: number;
        let e: _PolyhedralBoundedSolidEdge;
        let h1: _PolyhedralBoundedSolidHalfEdge | null;
        let h2: _PolyhedralBoundedSolidHalfEdge | null;
        let f: _PolyhedralBoundedSolidFace;
        let l: _PolyhedralBoundedSolidLoop;

        for (i = 0; i < solid.getEdgesList().size(); i++) {
            e = solid.getEdgesList().get(i)!;
            h1 = e.rightHalf;
            h2 = e.leftHalf;
            if (h1 === null || h2 === null) {
                Logger.reportMessage(solid, VSDK.WARNING, "validateTopologicalIntegrity", "Edge with null halfedge!");
                return false;
            }
            if (h1.parentLoop.parentFace.parentSolid !== h2.parentLoop.parentFace.parentSolid) {
                Logger.reportMessage(
                    solid,
                    VSDK.WARNING,
                    "validateTopologicalIntegrity",
                    "Edge belonging to two different solids!",
                );
                return false;
            }
        }

        const edgeCount = new Int32Array(solid.getEdgesList().size());
        for (i = 0; i < edgeCount.length; i++) {
            edgeCount[i] = 0;
        }

        for (i = 0; i < solid.getPolygonsList().size(); i++) {
            f = solid.getPolygonsList().get(i)!;
            for (j = 0; j < f.boundariesList.size(); j++) {
                let he: _PolyhedralBoundedSolidHalfEdge | null;
                l = f.boundariesList.get(j)!;

                he = l.boundaryStartHalfEdge;
                if (he === null) {
                    Logger.reportMessage(
                        solid,
                        VSDK.WARNING,
                        "validateTopologicalIntegrity",
                        "Loop without starting halfedge\n" + "Offending solid:\n" + solid.toString(),
                    );
                    return false;
                }
                const heStart = he;
                do {
                    he = he!.next();
                    if (he === null) {
                        Logger.reportMessage(solid, VSDK.WARNING, "validateTopologicalIntegrity", "Not closed loop!");
                        return false;
                    }

                    for (k = 0; k < edgeCount.length; k++) {
                        if (he.parentEdge === solid.getEdgesList().get(k)) {
                            edgeCount[k]!++;
                            break;
                        }
                    }
                } while (he !== heStart);
            }
        }

        for (i = 0; i < edgeCount.length; i++) {
            if (edgeCount[i] !== 2) {
                Logger.reportMessage(
                    solid,
                    VSDK.WARNING,
                    "validateTopologicalIntegrity",
                    "Edges with different halfedges than 2!",
                );
                return false;
            }
        }

        return true;
    }

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
    public static validateBackPointers(solid: PolyhedralBoundedSolid, msg: StringBuilder): boolean {
        let ok = true;
        let i: number;
        let j: number;
        let k: number;

        for (i = 0; i < solid.getPolygonsList().size(); i++) {
            const face = solid.getPolygonsList().get(i)!;
            for (j = 0; j < face.boundariesList.size(); j++) {
                const loop = face.boundariesList.get(j)!;
                if (loop.boundaryStartHalfEdge === null || !loop.halfEdgesList.contains(loop.boundaryStartHalfEdge)) {
                    msg.append("  - Face [")
                        .append(String(face.id))
                        .append("] has a loop whose start half-edge is not")
                        .append(" a member of the loop.\n");
                    ok = false;
                }
                for (k = 0; k < loop.halfEdgesList.size(); k++) {
                    if (loop.halfEdgesList.get(k)!.parentLoop !== loop) {
                        msg.append("  - Face [")
                            .append(String(face.id))
                            .append("] has a half-edge whose parent loop")
                            .append(" is another loop.\n");
                        ok = false;
                        break;
                    }
                }
            }
        }

        for (i = 0; i < solid.getVerticesList().size(); i++) {
            const v: _PolyhedralBoundedSolidVertex = solid.getVerticesList().get(i)!;
            const he: _PolyhedralBoundedSolidHalfEdge | null = v.emanatingHalfEdge;
            if (he === null) {
                msg.append("  - Vertex [").append(String(v.id)).append("] has no emanating half-edge.\n");
                ok = false;
            } else if (he.startingVertex !== v) {
                msg.append("  - Vertex [")
                    .append(String(v.id))
                    .append("] emanating half-edge starts at another")
                    .append(" vertex.\n");
                ok = false;
            } else if (he.parentLoop === null || !he.parentLoop.halfEdgesList.contains(he)) {
                msg.append("  - Vertex [")
                    .append(String(v.id))
                    .append("] emanating half-edge is not a member of its")
                    .append(" loop.\n");
                ok = false;
            }
        }
        return ok;
    }
}
