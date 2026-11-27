//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =
//= [.wMANT2008] Mantyla Martti. "Personal Home Page", <<shar>> archive     =
//=     containing the C programs from [MANT1988]. Available at             =
//=     http://www.cs.hut.fi/~mam . Last visited April 12 / 2008.           =

import { VSDK } from "../../../../common/VSDK.js";
import { VSDKFatalException } from "../../../../common/VSDKFatalException.js";
import { Logger } from "../../../../common/logging/Logger.js";
import { Vector3Dd } from "../../../../common/linealAlgebra/Vector3Dd.js";
import type { InfinitePlane } from "../../surface/InfinitePlane.js";
import type { _PolyhedralBoundedSolidEdge } from "./nodes/_PolyhedralBoundedSolidEdge.js";
import type { _PolyhedralBoundedSolidFace } from "./nodes/_PolyhedralBoundedSolidFace.js";
import type { _PolyhedralBoundedSolidHalfEdge } from "./nodes/_PolyhedralBoundedSolidHalfEdge.js";
import type { _PolyhedralBoundedSolidLoop } from "./nodes/_PolyhedralBoundedSolidLoop.js";
import type { _PolyhedralBoundedSolidVertex } from "./nodes/_PolyhedralBoundedSolidVertex.js";
import type { PolyhedralBoundedSolid } from "./PolyhedralBoundedSolid.js";
import { PolyhedralBoundedSolidEulerOperators } from "./PolyhedralBoundedSolidEulerOperators.js";
import { PolyhedralBoundedSolidGeometricValidator } from "./PolyhedralBoundedSolidGeometricValidator.js";
import { PolyhedralBoundedSolidNumericPolicy, type ToleranceContext } from "./PolyhedralBoundedSolidNumericPolicy.js";
import { PolyhedralBoundedSolidRepair } from "./PolyhedralBoundedSolidRepair.js";

/**
Contains topology-editing operations for `PolyhedralBoundedSolid` that are
built on Euler operators.
*/
export class PolyhedralBoundedSolidTopologyEditing {
    private constructor() {}

    private static remakeLoopBoundaryStartHalfEdgesReferences(solid: PolyhedralBoundedSolid): void {
        let i: number;
        let j: number;

        for (i = 0; i < solid.getPolygonsList().size(); i++) {
            const face = solid.getPolygonsList().get(i)!;
            for (j = 0; j < face.boundariesList.size(); j++) {
                const loop = face.boundariesList.get(j)!;
                loop.parentFace = face;
                if (loop.halfEdgesList.size() > 0) {
                    loop.boundaryStartHalfEdge = loop.halfEdgesList.get(0);
                } else {
                    loop.boundaryStartHalfEdge = null;
                }
            }
        }
    }

    /**
    After section [MANT1988].12.4.2 and program [MANT1988].12.9.
    @param solid target solid instance.
    @param faceId face id to glue.

    Java overload `loopGlue(PolyhedralBoundedSolid, _PolyhedralBoundedSolidFace)`:
    glues two coincident loops from a face by applying Euler operators.
    @param face face containing at least two loops to glue.
    */
    public static loopGlue(solid: PolyhedralBoundedSolid, faceOrId: number | _PolyhedralBoundedSolidFace | null): void {
        if (typeof faceOrId === "number") {
            const faceId = faceOrId;
            const face = solid.findFace(faceId);
            if (face === null) {
                Logger.reportMessage(solid, VSDK.WARNING, "loopGlue", "Face " + faceId + " not found.");
                return;
            }
            PolyhedralBoundedSolidTopologyEditing.loopGlue(solid, face);
            return;
        }
        const face = faceOrId;

        if (face === null) {
            Logger.reportMessage(solid, VSDK.WARNING, "loopGlue", "Null face received.");
            return;
        }
        if (face.boundariesList.size() < 2) {
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "loopGlue",
                "Face " + face.id + " does not contain at least two loops.",
            );
            return;
        }

        //-----------------------------------------------------------------
        let h1: _PolyhedralBoundedSolidHalfEdge;
        let h2: _PolyhedralBoundedSolidHalfEdge;
        let h1next: _PolyhedralBoundedSolidHalfEdge;

        let gluePair: _PolyhedralBoundedSolidHalfEdge[] | null = null;
        let i: number;
        let j: number;
        for (i = 0; i < face.boundariesList.size() && gluePair === null; i++) {
            for (j = i + 1; j < face.boundariesList.size(); j++) {
                gluePair = PolyhedralBoundedSolidTopologyEditing.findMatchingLoopVertices(
                    solid,
                    face.boundariesList.get(i)!.boundaryStartHalfEdge,
                    face.boundariesList.get(j)!.boundaryStartHalfEdge,
                );
                if (gluePair !== null) {
                    break;
                }
            }
        }

        if (gluePair === null) {
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "loopGlue",
                "No matching starting vertex found between candidate loops.",
            );
            return;
        }
        h1 = gluePair[0]!;
        h2 = gluePair[1]!;

        if (
            PolyhedralBoundedSolidTopologyEditing.isDegenerateLoop(solid, h1.parentLoop) &&
            PolyhedralBoundedSolidTopologyEditing.isDegenerateLoop(solid, h2.parentLoop)
        ) {
            PolyhedralBoundedSolidTopologyEditing.removeLoop(face, h1.parentLoop);
            PolyhedralBoundedSolidTopologyEditing.removeLoop(face, h2.parentLoop);
            PolyhedralBoundedSolidTopologyEditing.remakeLoopBoundaryStartHalfEdgesReferences(solid);
            return;
        }
        // §9.5: A degenerate ring (size < 3) cannot be bridged by lmekr
        // without creating a self-loop edge. The gluePair search may return
        // either loop as h1 or h2 — check both to cover the swapped case.
        if (PolyhedralBoundedSolidTopologyEditing.isDegenerateLoop(solid, h2.parentLoop)) {
            PolyhedralBoundedSolidTopologyEditing.removeLoop(face, h2.parentLoop);
            PolyhedralBoundedSolidTopologyEditing.remakeLoopBoundaryStartHalfEdgesReferences(solid);
            return;
        }
        if (PolyhedralBoundedSolidTopologyEditing.isDegenerateLoop(solid, h1.parentLoop)) {
            PolyhedralBoundedSolidTopologyEditing.removeLoop(face, h1.parentLoop);
            PolyhedralBoundedSolidTopologyEditing.remakeLoopBoundaryStartHalfEdgesReferences(solid);
            return;
        }

        PolyhedralBoundedSolidEulerOperators.lmekr(solid, h1, h2);
        PolyhedralBoundedSolidEulerOperators.lkev(solid, h1.previous(), h2.previous());

        while (h1.next() !== h2) {
            h1next = h1.next()!;
            PolyhedralBoundedSolidEulerOperators.lmef(solid, h1.next(), h1.previous(), solid.getMaxFaceId() + 1);
            PolyhedralBoundedSolidEulerOperators.lkev(solid, h1.next(), h1.next()!.mirrorHalfEdge());
            PolyhedralBoundedSolidEulerOperators.lkef(solid, h1.mirrorHalfEdge()!, h1);
            h1 = h1next;
        }
        PolyhedralBoundedSolidEulerOperators.lkef(solid, h1.mirrorHalfEdge()!, h1);
        PolyhedralBoundedSolidTopologyEditing.remakeLoopBoundaryStartHalfEdgesReferences(solid);
    }

    /**
    Modifies ids for current solid's vertices, edges, faces and half-edges to
    make them consecutive from 1.
    @param solid target solid instance.
    */
    public static compactIds(solid: PolyhedralBoundedSolid): void {
        let i: number;
        let j: number;

        for (i = 0; i < solid.getVerticesList().size(); i++) {
            solid.getVerticesList().get(i)!.id = i + 1;
        }
        solid.setMaxVertexId(i);
        for (i = 0; i < solid.getEdgesList().size(); i++) {
            solid.getEdgesList().get(i)!.id = i + 1;
        }

        let k = 1;
        for (i = 0; i < solid.getPolygonsList().size(); i++) {
            const face = solid.getPolygonsList().get(i)!;
            face.id = i + 1;
            for (j = 0; j < face.boundariesList.size(); j++) {
                let he: _PolyhedralBoundedSolidHalfEdge | null;

                const loop = face.boundariesList.get(j)!;

                he = loop.boundaryStartHalfEdge;
                if (he === null) {
                    continue;
                }
                const heStart = he;
                do {
                    he!.id = k;
                    k++;
                    he = he!.next();
                    if (he === null) {
                        break;
                    }
                } while (he !== heStart);
            }
        }
        solid.setMaxFaceId(i);
    }

    /**
    Finds a pair of coincident vertices between two loops so `loopGlue` can
    start from a geometrically meaningful bridge instead of assuming each loop
    starts at the matching vertex.

    This helper is not part of the original [MANT1988] text; it was added to
    make the implementation more robust when intermediate Boolean topology
    leaves valid loops with arbitrary boundary start half-edges.
    @param solid target solid instance.
    @param first start half-edge of the first loop.
    @param second start half-edge of the second loop.
    @return pair of matching half-edges, or null when no match exists.
    */
    private static findMatchingLoopVertices(
        solid: PolyhedralBoundedSolid,
        first: _PolyhedralBoundedSolidHalfEdge | null,
        second: _PolyhedralBoundedSolidHalfEdge | null,
    ): _PolyhedralBoundedSolidHalfEdge[] | null {
        const numericContext = PolyhedralBoundedSolidNumericPolicy.forSolid(solid);
        if (first === null || second === null) {
            return null;
        }

        let h1: _PolyhedralBoundedSolidHalfEdge = first;
        do {
            let h2: _PolyhedralBoundedSolidHalfEdge = second;
            do {
                if (h1.vertexPositionMatch(h2, numericContext.bigEpsilon())) {
                    return [h1, h2];
                }
                h2 = h2.next()!;
            } while (h2 !== second);
            h1 = h1.next()!;
        } while (h1 !== first);

        return null;
    }

    /**
    Detects loops that do not contain enough distinct geometric vertices to
    describe an area.

    This helper is not part of the original [MANT1988] text; it was added to
    make the implementation more robust when cleanup stages produce collapsed
    loops that should be removed instead of glued as regular rings.
    @param solid target solid instance.
    @param loop loop to evaluate.
    @return true when the loop is degenerate, false otherwise.
    */
    private static isDegenerateLoop(solid: PolyhedralBoundedSolid, loop: _PolyhedralBoundedSolidLoop | null): boolean {
        if (loop === null || loop.halfEdgesList.size() < 3) {
            return true;
        }

        const numericContext = PolyhedralBoundedSolidNumericPolicy.forSolid(solid);
        let distinctCount = 0;
        let i: number;
        for (i = 0; i < loop.halfEdgesList.size(); i++) {
            let j: number;
            for (j = 0; j < i; j++) {
                if (
                    PolyhedralBoundedSolidNumericPolicy.pointsCoincident(
                        loop.halfEdgesList.get(i)!.startingVertex.position,
                        loop.halfEdgesList.get(j)!.startingVertex.position,
                        numericContext,
                    )
                ) {
                    // Self-touching loop: vertex i shares position with vertex j.
                    // This creates a figure-8 boundary that cannot bound a valid face.
                    Logger.reportMessage(
                        solid,
                        VSDK.WARNING,
                        "isDegenerateLoop",
                        "finish: skipped degenerate loop with self-touching boundary " +
                            "(vertex " +
                            loop.halfEdgesList.get(i)!.startingVertex.id +
                            " coincides with vertex " +
                            loop.halfEdgesList.get(j)!.startingVertex.id +
                            ")",
                    );
                    return true;
                }
            }
            distinctCount++;
        }
        // A valid face boundary needs at least 3 geometrically distinct vertices.
        return distinctCount < 3;
    }

    /**
    Removes a loop reference from a face without applying a full Euler
    operator, for cases where the loop has already collapsed geometrically.

    This helper is not part of the original [MANT1988] text; it was added to
    make the implementation more robust around degenerate intermediate loops
    created by Boolean cleanup.
    @param face owner face.
    @param loop loop to remove.
    */
    private static removeLoop(face: _PolyhedralBoundedSolidFace, loop: _PolyhedralBoundedSolidLoop): void {
        let i: number;
        for (i = 0; i < face.boundariesList.size(); i++) {
            if (face.boundariesList.get(i) === loop) {
                face.boundariesList.remove(i);
                return;
            }
        }
    }

    private static planesCoincidentIgnoringOrientation(
        a: InfinitePlane | null,
        b: InfinitePlane | null,
        tolerance: number,
    ): boolean {
        let a1: number;
        let b1: number;
        let c1: number;
        let d1: number;
        let a2: number;
        let b2: number;
        let c2: number;
        let d2: number;

        if (a === null || b === null) {
            return false;
        }

        a1 = a.getA();
        b1 = a.getB();
        c1 = a.getC();
        d1 = a.getD();
        a2 = b.getA();
        b2 = b.getB();
        c2 = b.getC();
        d2 = b.getD();

        const l1 = Math.sqrt(a1 * a1 + b1 * b1 + c1 * c1);
        const l2 = Math.sqrt(a2 * a2 + b2 * b2 + c2 * c2);
        if (l1 <= tolerance || l2 <= tolerance) {
            return false;
        }

        a1 /= l1;
        b1 /= l1;
        c1 /= l1;
        d1 /= l1;
        a2 /= l2;
        b2 /= l2;
        c2 /= l2;
        d2 /= l2;

        const sameOrientation =
            Math.abs(a2 - a1) <= tolerance &&
            Math.abs(b2 - b1) <= tolerance &&
            Math.abs(c2 - c1) <= tolerance &&
            Math.abs(d2 - d1) <= tolerance;

        const oppositeOrientation =
            Math.abs(a2 + a1) <= tolerance &&
            Math.abs(b2 + b1) <= tolerance &&
            Math.abs(c2 + c1) <= tolerance &&
            Math.abs(d2 + d1) <= tolerance;

        return sameOrientation || oppositeOrientation;
    }

    private static loopsCoincidentFrom(
        startA: _PolyhedralBoundedSolidHalfEdge,
        startB: _PolyhedralBoundedSolidHalfEdge,
        reverse: boolean,
        numericContext: ToleranceContext,
    ): boolean {
        let heA: _PolyhedralBoundedSolidHalfEdge;
        let heB: _PolyhedralBoundedSolidHalfEdge;

        heA = startA;
        heB = startB;
        do {
            if (
                !PolyhedralBoundedSolidNumericPolicy.pointsCoincident(
                    heA.startingVertex.position,
                    heB.startingVertex.position,
                    numericContext,
                )
            ) {
                return false;
            }
            heA = heA.next()!;
            heB = reverse ? heB.previous()! : heB.next()!;
        } while (heA !== startA);

        return true;
    }

    private static loopsCoincident(
        a: _PolyhedralBoundedSolidLoop | null,
        b: _PolyhedralBoundedSolidLoop | null,
        numericContext: ToleranceContext,
    ): boolean {
        let i: number;
        let scanB: _PolyhedralBoundedSolidHalfEdge;

        if (a === null || b === null || a.boundaryStartHalfEdge === null || b.boundaryStartHalfEdge === null) {
            return false;
        }
        if (a.halfEdgesList.size() !== b.halfEdgesList.size()) {
            return false;
        }

        const startA = a.boundaryStartHalfEdge;
        scanB = b.boundaryStartHalfEdge;
        for (i = 0; i < b.halfEdgesList.size(); i++) {
            if (
                PolyhedralBoundedSolidNumericPolicy.pointsCoincident(
                    startA.startingVertex.position,
                    scanB.startingVertex.position,
                    numericContext,
                )
            ) {
                if (
                    PolyhedralBoundedSolidTopologyEditing.loopsCoincidentFrom(startA, scanB, false, numericContext) ||
                    PolyhedralBoundedSolidTopologyEditing.loopsCoincidentFrom(startA, scanB, true, numericContext)
                ) {
                    return true;
                }
            }
            scanB = scanB.next()!;
        }

        return false;
    }

    /**
    Detects the duplicated coplanar-ring configuration that can appear after
    boolean result integration. The reorganization is meant to expose the
    pair of coincident loops that `loopglue` consumes in section
    [MANT1988].12.4.2, as required by the maximal-face cleanup from section
    [MANT1988].15.5 and the finishing stage of program [MANT1988].15.15.
    @param solid target solid instance.
    @param multiLoopFace multi-loop candidate face.
    @param simpleFace single-loop candidate face.
    @param numericContext numeric tolerance context.
    @return true when a reduction was applied, false otherwise.
    */
    private static reduceCoincidentSimpleFaceOnMultiLoopFace(
        solid: PolyhedralBoundedSolid,
        multiLoopFace: _PolyhedralBoundedSolidFace | null,
        simpleFace: _PolyhedralBoundedSolidFace | null,
        numericContext: ToleranceContext,
    ): boolean {
        let i: number;

        if (multiLoopFace === null || simpleFace === null || multiLoopFace === simpleFace) {
            return false;
        }
        if (multiLoopFace.boundariesList.size() < 2 || simpleFace.boundariesList.size() !== 1) {
            return false;
        }
        const multiLoopPlane = multiLoopFace.getContainingPlane();
        const simplePlane = simpleFace.getContainingPlane();
        if (multiLoopPlane === null || simplePlane === null) {
            return false;
        }
        if (
            !PolyhedralBoundedSolidTopologyEditing.planesCoincidentIgnoringOrientation(
                multiLoopPlane,
                simplePlane,
                numericContext.epsilon(),
            )
        ) {
            return false;
        }

        const simpleLoop = simpleFace.boundariesList.get(0)!;
        const coincidentLoops: _PolyhedralBoundedSolidLoop[] = [];
        for (i = 0; i < multiLoopFace.boundariesList.size(); i++) {
            const candidate = multiLoopFace.boundariesList.get(i)!;
            if (PolyhedralBoundedSolidTopologyEditing.loopsCoincident(candidate, simpleLoop, numericContext)) {
                coincidentLoops.push(candidate);
            }
        }

        if (coincidentLoops.length < 2) {
            return false;
        }

        const outerGlueLoop = coincidentLoops[0]!;
        const duplicateGlueLoop = coincidentLoops[1]!;
        const loopsToMove: _PolyhedralBoundedSolidLoop[] = [];
        for (i = 0; i < multiLoopFace.boundariesList.size(); i++) {
            const candidate = multiLoopFace.boundariesList.get(i)!;
            if (candidate !== outerGlueLoop && candidate !== duplicateGlueLoop) {
                loopsToMove.push(candidate);
            }
        }

        if (loopsToMove.length === 0) {
            return false;
        }

        for (i = 0; i < loopsToMove.length; i++) {
            if (!PolyhedralBoundedSolidEulerOperators.lringmv(solid, loopsToMove[i]!, simpleFace, false)) {
                return false;
            }
        }

        if (multiLoopFace.boundariesList.size() !== 2) {
            return false;
        }

        PolyhedralBoundedSolidEulerOperators.lringmv(solid, outerGlueLoop, multiLoopFace, true);
        PolyhedralBoundedSolidTopologyEditing.loopGlue(solid, multiLoopFace.id);
        return true;
    }

    private static hasCoincidentMultiLoopReductionCandidate(
        faceA: _PolyhedralBoundedSolidFace,
        faceB: _PolyhedralBoundedSolidFace,
        numericContext: ToleranceContext,
    ): boolean {
        return (
            PolyhedralBoundedSolidTopologyEditing.countCoincidentLoops(faceA, faceB, numericContext) >= 2 ||
            PolyhedralBoundedSolidTopologyEditing.countCoincidentLoops(faceB, faceA, numericContext) >= 2
        );
    }

    private static countCoincidentLoops(
        multiLoopFace: _PolyhedralBoundedSolidFace | null,
        simpleFace: _PolyhedralBoundedSolidFace | null,
        numericContext: ToleranceContext,
    ): number {
        let i: number;
        let coincidentCount: number;

        if (
            multiLoopFace === null ||
            simpleFace === null ||
            multiLoopFace.boundariesList.size() < 2 ||
            simpleFace.boundariesList.size() !== 1
        ) {
            return 0;
        }

        const simpleLoop = simpleFace.boundariesList.get(0)!;
        coincidentCount = 0;
        for (i = 0; i < multiLoopFace.boundariesList.size(); i++) {
            if (
                PolyhedralBoundedSolidTopologyEditing.loopsCoincident(
                    multiLoopFace.boundariesList.get(i)!,
                    simpleLoop,
                    numericContext,
                )
            ) {
                coincidentCount++;
            }
        }
        return coincidentCount;
    }

    private static removeEdgeRecord(solid: PolyhedralBoundedSolid, edge: _PolyhedralBoundedSolidEdge | null): void {
        let i: number;

        if (edge === null) {
            return;
        }

        for (i = 0; i < solid.getEdgesList().size(); i++) {
            if (solid.getEdgesList().get(i) === edge) {
                solid.getEdgesList().remove(i);
                return;
            }
        }
    }

    private static removeEmptyLoopAndFaceIfNeeded(
        solid: PolyhedralBoundedSolid,
        face: _PolyhedralBoundedSolidFace | null,
        loop: _PolyhedralBoundedSolidLoop | null,
    ): void {
        if (face === null || loop === null) {
            return;
        }

        if (loop.halfEdgesList.size() === 0) {
            face.boundariesList.locateWindowAtElem(loop);
            face.boundariesList.removeElemAtWindow();
        }

        if (face.boundariesList.size() === 0) {
            solid.getPolygonsList().locateWindowAtElem(face);
            solid.getPolygonsList().removeElemAtWindow();
        }
    }

    /**
    Repairs a duplicated geometric strut represented as two consecutive
    topological edges over the same geometric segment, each mirrored on
    different faces.

    This helper is not part of the original [MANT1988] text; it was added to
    make the implementation more robust when `maximizeFaces` encounters the
    kind of duplicated geometric strut that can survive Boolean connect/finish.
    @param solid target solid instance.
    @param first first consecutive half-edge.
    @param second second consecutive half-edge.
    @param iteration current cleanup iteration.
    @return true when a repair was applied, false otherwise.
    */
    private static zipConsecutiveGeometricDanglingEdgePair(
        solid: PolyhedralBoundedSolid,
        first: _PolyhedralBoundedSolidHalfEdge,
        second: _PolyhedralBoundedSolidHalfEdge,
        _iteration: number,
    ): boolean {
        const firstMirror = first.mirrorHalfEdge();
        const secondMirror = second.mirrorHalfEdge();
        if (
            firstMirror === null ||
            secondMirror === null ||
            firstMirror.parentLoop === null ||
            secondMirror.parentLoop === null ||
            firstMirror.parentLoop.parentFace === null ||
            secondMirror.parentLoop.parentFace === null ||
            firstMirror.parentLoop.parentFace === secondMirror.parentLoop.parentFace
        ) {
            return false;
        }

        const keptEdge = first.parentEdge!;
        const droppedEdge = second.parentEdge;
        const degenerateLoop = first.parentLoop;
        const degenerateFace = degenerateLoop.parentFace;

        degenerateLoop.unlistHalfEdge(first);
        degenerateLoop.unlistHalfEdge(second);

        PolyhedralBoundedSolidTopologyEditing.removeEdgeRecord(solid, droppedEdge);
        keptEdge.rightHalf = firstMirror;
        keptEdge.leftHalf = secondMirror;
        firstMirror.parentEdge = keptEdge;
        secondMirror.parentEdge = keptEdge;

        PolyhedralBoundedSolidEulerOperators.lkef(solid, firstMirror, secondMirror);
        PolyhedralBoundedSolidTopologyEditing.removeEmptyLoopAndFaceIfNeeded(solid, degenerateFace, degenerateLoop);
        return true;
    }

    /**
    Searches the solid for a duplicated geometric strut that can be repaired
    by `zipConsecutiveGeometricDanglingEdgePair`.

    This helper is not part of the original [MANT1988] text; it was added to
    make the implementation more robust by broadening maximal-face cleanup
    beyond the exact topological cases described in the book.
    @param solid target solid instance.
    @param numericContext numeric tolerance context.
    @param iteration current cleanup iteration.
    @return true when a repair was applied, false otherwise.
    */
    private static zipConsecutiveGeometricDanglingEdgePairs(
        solid: PolyhedralBoundedSolid,
        numericContext: ToleranceContext,
        iteration: number,
    ): boolean {
        let i: number;
        let j: number;
        let first: _PolyhedralBoundedSolidHalfEdge;
        let second: _PolyhedralBoundedSolidHalfEdge | null;
        let firstMirror: _PolyhedralBoundedSolidHalfEdge | null;
        let secondMirror: _PolyhedralBoundedSolidHalfEdge | null;
        let firstMirrorPlane: InfinitePlane | null;
        let secondMirrorPlane: InfinitePlane | null;

        for (i = 0; i < solid.getPolygonsList().size(); i++) {
            const face = solid.getPolygonsList().get(i)!;
            for (j = 0; j < face.boundariesList.size(); j++) {
                const loop = face.boundariesList.get(j)!;
                let k: number;

                if (loop.halfEdgesList.size() < 2) {
                    continue;
                }

                for (k = 0; k < loop.halfEdgesList.size(); k++) {
                    first = loop.halfEdgesList.get(k)!;
                    second = first.next();
                    if (
                        second === null ||
                        first === second ||
                        first.parentEdge === null ||
                        second.parentEdge === null ||
                        first.parentEdge === second.parentEdge ||
                        first.startingVertex === null ||
                        second.startingVertex === null
                    ) {
                        continue;
                    }

                    if (
                        !PolyhedralBoundedSolidNumericPolicy.pointsCoincident(
                            first.startingVertex.position,
                            second.startingVertex.position,
                            numericContext,
                        )
                    ) {
                        continue;
                    }

                    firstMirror = first.mirrorHalfEdge();
                    secondMirror = second.mirrorHalfEdge();
                    if (
                        firstMirror === null ||
                        secondMirror === null ||
                        firstMirror.parentLoop === null ||
                        secondMirror.parentLoop === null ||
                        firstMirror.parentLoop.parentFace === null ||
                        secondMirror.parentLoop.parentFace === null ||
                        firstMirror.parentLoop.parentFace === secondMirror.parentLoop.parentFace
                    ) {
                        continue;
                    }

                    firstMirrorPlane = firstMirror.parentLoop.parentFace.getContainingPlane();
                    secondMirrorPlane = secondMirror.parentLoop.parentFace.getContainingPlane();
                    if (firstMirrorPlane === null || secondMirrorPlane === null) {
                        continue;
                    }
                    if (
                        !PolyhedralBoundedSolidTopologyEditing.planesCoincidentIgnoringOrientation(
                            firstMirrorPlane,
                            secondMirrorPlane,
                            numericContext.epsilon(),
                        )
                    ) {
                        continue;
                    }

                    return PolyhedralBoundedSolidTopologyEditing.zipConsecutiveGeometricDanglingEdgePair(
                        solid,
                        first,
                        second,
                        iteration,
                    );
                }
            }
        }
        return false;
    }

    private static remakeEmanatingHalfedgesReferences(solid: PolyhedralBoundedSolid): void {
        PolyhedralBoundedSolidRepair.remakeEmanatingHalfEdgeReferences(solid);
    }

    /**
    §9.3 planarity guard: collects vertices from both loops of an edge's two
    adjacent faces and checks that all combined points are coplanar. Prevents
    {@code maximizeFaces} from merging two faces whose union would be
    non-planar even though their individual planes nominally overlap.
    */
    private static wouldMergedFaceBeCoplanar(
        rightHalf: _PolyhedralBoundedSolidHalfEdge,
        leftHalf: _PolyhedralBoundedSolidHalfEdge,
        numericContext: ToleranceContext,
    ): boolean {
        const points: Vector3Dd[] = [];
        let start: _PolyhedralBoundedSolidHalfEdge | null;
        let cur: _PolyhedralBoundedSolidHalfEdge | null;

        start = rightHalf.parentLoop.boundaryStartHalfEdge;
        cur = start;
        if (cur !== null) {
            do {
                if (cur.startingVertex !== null && cur.startingVertex.position !== null) {
                    points.push(cur.startingVertex.position);
                }
                cur = cur.next();
            } while (cur !== null && cur !== start);
        }
        start = leftHalf.parentLoop.boundaryStartHalfEdge;
        cur = start;
        if (cur !== null) {
            do {
                if (cur.startingVertex !== null && cur.startingVertex.position !== null) {
                    points.push(cur.startingVertex.position);
                }
                cur = cur.next();
            } while (cur !== null && cur !== start);
        }
        return PolyhedralBoundedSolidGeometricValidator.validateFacePointsAreCoplanar(points, numericContext);
    }

    /**
    Removes all "inessential" edges of current solid (i.e. edges that
    separates two coplanar faces, or that occurs just in a single face).
    This is an answer to problem [MANT1988].15.2.
    For some operations on solid polyhedra such as boolean set operations,
    it is required that faces of solids be "maximal", i.e. that all coplanar
    neighbor faces have been combined, and all "inessential" edges have been
    removed, as noted on section [MANT1988].15.5. Current implementation also
    performs an additional coplanar-ring reduction so that coincident sheets
    can be eliminated with `loopglue`, consistent with section
    [MANT1988].12.4.2 and the result-finishing strategy of program
    [MANT1988].15.15.
    @param solid target solid instance.
    */
    public static maximizeFaces(solid: PolyhedralBoundedSolid): void {
        let i: number;
        let j: number;
        let e: _PolyhedralBoundedSolidEdge;
        let he: _PolyhedralBoundedSolidHalfEdge | null;
        let a: InfinitePlane | null;
        let b: InfinitePlane | null;
        let p0: Vector3Dd;
        let p1: Vector3Dd;
        let p2: Vector3Dd;
        let heStart: _PolyhedralBoundedSolidHalfEdge | null;
        let restart: boolean;
        let numericContext: ToleranceContext;
        let iteration: number;

        restart = true;
        iteration = 0;
        while (restart) {
            restart = false;
            iteration++;
            numericContext = PolyhedralBoundedSolidNumericPolicy.forSolid(solid);
            PolyhedralBoundedSolidTopologyEditing.remakeEmanatingHalfedgesReferences(solid);
            //- Collapse residual line-faces --------------------------------
            for (i = 0; i < solid.getPolygonsList().size(); i++) {
                const face = solid.getPolygonsList().get(i)!;
                if (face.boundariesList.size() !== 1) {
                    continue;
                }
                const loop = face.boundariesList.get(0)!;
                if (loop.halfEdgesList.size() !== 2) {
                    continue;
                }
                const collapseHe = loop.boundaryStartHalfEdge;
                if (collapseHe === null) {
                    continue;
                }
                const neighborHe = collapseHe.mirrorHalfEdge();
                if (
                    collapseHe.parentEdge === null ||
                    neighborHe === null ||
                    neighborHe.parentLoop === null ||
                    neighborHe.parentLoop.parentFace === null ||
                    neighborHe.parentLoop.parentFace === face
                ) {
                    continue;
                }
                PolyhedralBoundedSolidEulerOperators.lkef(solid, collapseHe, neighborHe);
                restart = true;
                break;
            }
            if (restart) {
                continue;
            }

            //- Eliminate null edges --------------------------------------
            for (i = 0; i < solid.getEdgesList().size(); i++) {
                e = solid.getEdgesList().get(i)!;
                p1 = e.rightHalf!.startingVertex.position;
                p2 = e.leftHalf!.startingVertex.position;
                if (PolyhedralBoundedSolidNumericPolicy.pointsCoincident(p1, p2, numericContext)) {
                    PolyhedralBoundedSolidEulerOperators.lkev(solid, e.rightHalf, e.leftHalf);
                    restart = true;
                    break;
                }
            }
            if (restart) {
                continue;
            }

            //- Zip duplicated geometric struts ---------------------------
            if (
                PolyhedralBoundedSolidTopologyEditing.zipConsecutiveGeometricDanglingEdgePairs(
                    solid,
                    numericContext,
                    iteration,
                )
            ) {
                restart = true;
                continue;
            }

            //- Join coplanar faces ---------------------------------------
            for (i = 0; i < solid.getEdgesList().size(); i++) {
                e = solid.getEdgesList().get(i)!;
                const rightHalf = e.rightHalf!;
                const leftHalf = e.leftHalf!;
                a = rightHalf.parentLoop.parentFace.getContainingPlane();
                b = leftHalf.parentLoop.parentFace.getContainingPlane();
                if (
                    rightHalf.parentLoop.parentFace === leftHalf.parentLoop.parentFace &&
                    rightHalf.parentLoop !== leftHalf.parentLoop
                ) {
                    // Case 1: need to remove an edge separating to
                    // different coplanar faces (join faces). Order doesn't
                    // matter.
                    PolyhedralBoundedSolidEulerOperators.lkemr(solid, rightHalf, leftHalf);
                    restart = true;
                    break;
                } else if (
                    a !== null &&
                    b !== null &&
                    a.overlapsWith(b, numericContext.epsilon()) &&
                    rightHalf.parentLoop !== leftHalf.parentLoop
                ) {
                    if (
                        PolyhedralBoundedSolidTopologyEditing.hasCoincidentMultiLoopReductionCandidate(
                            rightHalf.parentLoop.parentFace,
                            leftHalf.parentLoop.parentFace,
                            numericContext,
                        )
                    ) {
                        continue;
                    }
                    // §9.3 guard: even though both face planes overlap, the
                    // merged polygon may be non-planar due to floating-point
                    // drift (e.g. triangulated curved surfaces). Skip if the
                    // combined vertex set is not coplanar.
                    if (
                        !PolyhedralBoundedSolidTopologyEditing.wouldMergedFaceBeCoplanar(
                            rightHalf,
                            leftHalf,
                            numericContext,
                        )
                    ) {
                        continue;
                    }
                    PolyhedralBoundedSolidEulerOperators.lkef(solid, rightHalf, leftHalf);
                    restart = true;
                    break;
                } else if (
                    rightHalf.parentLoop.parentFace === leftHalf.parentLoop.parentFace &&
                    rightHalf.parentLoop === leftHalf.parentLoop &&
                    (leftHalf === rightHalf.next() || rightHalf === leftHalf.next())
                ) {
                    // Case 3:. Need to remove a dangling edge, with two
                    // half-edges lying over the same face. Do not remove any
                    // face, rather, remove the dangling edge and its dangling
                    // vertex. To test, use object from figure [MANT1988].15.1.
                    // or code from SimpleTestGeometryLibrary method
                    // createTestObjectPairMANT1988_15_1
                    if (leftHalf === rightHalf.next()) {
                        heStart = leftHalf;
                    } else {
                        heStart = rightHalf;
                    }
                    PolyhedralBoundedSolidEulerOperators.lkev(solid, heStart, heStart.mirrorHalfEdge());
                    restart = true;
                    break;
                } else if (
                    rightHalf.parentLoop.parentFace === leftHalf.parentLoop.parentFace &&
                    rightHalf.parentLoop === leftHalf.parentLoop &&
                    leftHalf !== rightHalf.next() &&
                    rightHalf !== leftHalf.next()
                ) {
                    // Case 4. Need to remove an edge on a self-intersecting
                    // loop, causing that loop to break on two rings. To test
                    // use "buildCsgTest4" pair on
                    // PolyhedralBoundedSolidModelingTools testsuite program
                    // (union of two L-shaped boxes to form a hollowed brick).
                    // It is important to break the loops in such a way that
                    // bigger loop be the first loop, and smaller loop is the
                    // inner ring.

                    // Estimate the size of semiloop starting at e.leftHalf
                    const minmax = solid.getMinMax();
                    let min = new Vector3Dd(minmax[3]!, minmax[4]!, minmax[5]!);
                    let max = new Vector3Dd(minmax[0]!, minmax[1]!, minmax[2]!);
                    let p: Vector3Dd;
                    heStart = leftHalf;
                    he = heStart;
                    do {
                        he = he!.next();
                        if (he === null) {
                            // Loop is not closed!
                            break;
                        }
                        p = he.startingVertex.position;
                        if (p.x() > max.x()) max = max.withX(p.x());
                        if (p.y() > max.y()) max = max.withY(p.y());
                        if (p.z() > max.z()) max = max.withZ(p.z());
                        if (p.x() < min.x()) min = min.withX(p.x());
                        if (p.y() < min.y()) min = min.withY(p.y());
                        if (p.z() < min.z()) min = min.withZ(p.z());
                    } while (he !== heStart && he !== rightHalf);
                    const leftDistance = Vector3Dd.distance(min, max);

                    // Estimate the size of semiloop starting at e.rightHalf
                    min = new Vector3Dd(minmax[3]!, minmax[4]!, minmax[5]!);
                    max = new Vector3Dd(minmax[0]!, minmax[1]!, minmax[2]!);
                    heStart = rightHalf;
                    he = heStart;
                    do {
                        he = he!.next();
                        if (he === null) {
                            // Loop is not closed!
                            break;
                        }
                        p = he.startingVertex.position;
                        if (p.x() > max.x()) max = max.withX(p.x());
                        if (p.y() > max.y()) max = max.withY(p.y());
                        if (p.z() > max.z()) max = max.withZ(p.z());
                        if (p.x() < min.x()) min = min.withX(p.x());
                        if (p.y() < min.y()) min = min.withY(p.y());
                        if (p.z() < min.z()) min = min.withZ(p.z());
                    } while (he !== heStart && he !== leftHalf);
                    const rightDistance = Vector3Dd.distance(min, max);

                    // Determine outer loop acording to major extent
                    let heOuter: _PolyhedralBoundedSolidHalfEdge;
                    let heInner: _PolyhedralBoundedSolidHalfEdge;

                    if (leftDistance > rightDistance) {
                        heOuter = leftHalf;
                        heInner = rightHalf;
                    } else {
                        heOuter = rightHalf;
                        heInner = leftHalf;
                    }
                    PolyhedralBoundedSolidEulerOperators.lkemr(solid, heInner, heOuter);
                    restart = true;
                    break;
                }
            }
            if (restart) {
                continue;
            }

            //- Merge coplanar overlapping faces when one lies entirely over
            //- another that already carries rings. This completes the
            //- "maximal face" reduction expected by [MANT1988].15.5.
            // Which faces have a plane is computed once per sweep (Stage G):
            // the sweep restarts as soon as a reduction changes the solid.
            const hasPlane: boolean[] = new Array<boolean>(solid.getPolygonsList().size()).fill(false);
            for (i = 0; i < hasPlane.length; i++) {
                hasPlane[i] = solid.getPolygonsList().get(i)!.getContainingPlane() !== null;
            }
            for (i = 0; i < solid.getPolygonsList().size() && !restart; i++) {
                const faceA = solid.getPolygonsList().get(i)!;
                if (!hasPlane[i]) {
                    continue;
                }
                for (j = i + 1; j < solid.getPolygonsList().size(); j++) {
                    const faceB = solid.getPolygonsList().get(j)!;
                    if (!hasPlane[j]) {
                        continue;
                    }

                    if (
                        PolyhedralBoundedSolidTopologyEditing.reduceCoincidentSimpleFaceOnMultiLoopFace(
                            solid,
                            faceA,
                            faceB,
                            numericContext,
                        ) ||
                        PolyhedralBoundedSolidTopologyEditing.reduceCoincidentSimpleFaceOnMultiLoopFace(
                            solid,
                            faceB,
                            faceA,
                            numericContext,
                        )
                    ) {
                        restart = true;
                        break;
                    }
                }
            }
            if (restart) {
                continue;
            }

            //- Eliminate vertices between colinear edges -----------------
            let heMirror: _PolyhedralBoundedSolidHalfEdge | null;
            let v: _PolyhedralBoundedSolidVertex;
            let nedges: number;

            for (i = 0; i < solid.getVerticesList().size(); i++) {
                v = solid.getVerticesList().get(i)!;
                heStart = v.emanatingHalfEdge;
                if (heStart === null) {
                    continue;
                }
                he = heStart;
                nedges = 0;
                j = 0;
                do {
                    nedges++;
                    if (nedges > 2) break;

                    if (he === null) {
                        throw new VSDKFatalException("maximizeFaces: Inconsistent model! Null HalfEdge. Check.");
                    }

                    heMirror = he!.mirrorHalfEdge();
                    if (heMirror === null) {
                        nedges = 0;
                        continue;
                    }

                    he = heMirror.next();
                    if (he === null) {
                        throw new VSDKFatalException(
                            "maximizeFaces: Inconsistent model! HalfEdge without next. Check.",
                        );
                    }
                    j++;
                } while (he !== heStart);

                if (nedges === 2) {
                    p0 = heStart.startingVertex.position;
                    p1 = heStart.next()!.startingVertex.position.subtract(p0);
                    p2 = heStart.previous()!.startingVertex.position.subtract(p0);
                    if (PolyhedralBoundedSolidNumericPolicy.vectorsColinear(p1, p2, numericContext)) {
                        if (p1.dotProduct(p2) < 0) {
                            PolyhedralBoundedSolidEulerOperators.lkev(solid, heStart, heStart.mirrorHalfEdge());
                            restart = true;
                            break;
                        }
                    }
                }
            }
        }

        //- Eliminate rings with a single vertex --------------------------
        for (i = 0; i < solid.getPolygonsList().size(); i++) {
            const face = solid.getPolygonsList().get(i)!;

            const outerloophe = face.boundariesList.get(0)!.boundaryStartHalfEdge!;

            for (j = 1; j < face.boundariesList.size(); j++) {
                const loop = face.boundariesList.get(j)!;
                he = loop.boundaryStartHalfEdge!;
                if (he.parentEdge === null || loop.halfEdgesList.size() === 1) {
                    // Kill ring
                    const vtodelete = he.startingVertex;
                    PolyhedralBoundedSolidEulerOperators.lmekr(solid, outerloophe, he);

                    // Kill edge and vertex
                    let hej: _PolyhedralBoundedSolidHalfEdge | null;
                    hej = outerloophe;
                    do {
                        hej = hej!.next();
                        if (hej === null) {
                            // Loop is not closed!
                            break;
                        }
                        if (hej.startingVertex === vtodelete) {
                            PolyhedralBoundedSolidEulerOperators.lkev(solid, hej, hej.mirrorHalfEdge());
                            break;
                        }
                    } while (hej !== outerloophe);
                }
            }
        }
        // Here should be a code searching for faces inside faces ...
        PolyhedralBoundedSolidTopologyEditing.remakeEmanatingHalfedgesReferences(solid);
    }

    /**
    Removes edges whose two endpoints are geometrically coincident, merging
    the two vertices into one via {@code lkev}.  The repair is iterative:
    each pass restarts from the beginning of the edge list because {@code lkev}
    modifies the list.  Only edges where BOTH half-edge endpoints are within
    {@code context.bigEpsilon()} of each other are collapsed; the surviving
    vertex keeps the position of the half-edge's {@code startingVertex}.
    <p>
    This corresponds to the pre-processing step of [MANT1988].15 that ensures
    no geometrically degenerate edges exist before {@code setOpGenerate}.
    @param solid target solid instance.
    @param context tolerance context used to detect coincidence.
    @return number of edges collapsed.
    */
    public static weldCoincidentVertices(solid: PolyhedralBoundedSolid, context: ToleranceContext): number {
        let weldCount: number;
        let found: boolean;
        let i: number;
        let edge: _PolyhedralBoundedSolidEdge | null;
        let v1: _PolyhedralBoundedSolidVertex;
        let v2: _PolyhedralBoundedSolidVertex;

        weldCount = 0;
        do {
            found = false;
            for (i = 0; i < solid.getEdgesList().size(); i++) {
                edge = solid.getEdgesList().get(i);
                if (
                    edge === null ||
                    edge.rightHalf === null ||
                    edge.leftHalf === null ||
                    edge.rightHalf.startingVertex === null ||
                    edge.leftHalf.startingVertex === null
                ) {
                    continue;
                }
                v1 = edge.rightHalf.startingVertex;
                v2 = edge.leftHalf.startingVertex;
                if (PolyhedralBoundedSolidNumericPolicy.pointsCoincident(v1.position, v2.position, context)) {
                    PolyhedralBoundedSolidEulerOperators.lkev(solid, edge.rightHalf, edge.leftHalf);
                    weldCount++;
                    found = true;
                    break;
                }
            }
        } while (found);
        return weldCount;
    }
}
