//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =
//= [.wMANT2008] Mantyla Martti. "Personal Home Page", <<shar>> archive     =
//=     containing the C programs from [MANT1988]. Available at             =
//=     http://www.cs.hut.fi/~mam . Last visited April 12 / 2008.           =

import { PolyhedralBoundedSolidStatistics } from "../../../../common/statistics/PolyhedralBoundedSolidStatistics.js";
import { VSDK } from "../../../../common/VSDK.js";
import { VSDKFatalException } from "../../../../common/VSDKFatalException.js";
import { Logger } from "../../../../common/logging/Logger.js";
import type { Vector3Dd } from "../../../../common/linealAlgebra/Vector3Dd.js";
import { PolyhedralBoundedSolid } from "./PolyhedralBoundedSolid.js";
import { _PolyhedralBoundedSolidEdge } from "./nodes/_PolyhedralBoundedSolidEdge.js";
import { _PolyhedralBoundedSolidFace } from "./nodes/_PolyhedralBoundedSolidFace.js";
import { _PolyhedralBoundedSolidHalfEdge } from "./nodes/_PolyhedralBoundedSolidHalfEdge.js";
import { _PolyhedralBoundedSolidLoop } from "./nodes/_PolyhedralBoundedSolidLoop.js";
import { _PolyhedralBoundedSolidVertex } from "./nodes/_PolyhedralBoundedSolidVertex.js";

/**
Contains the implementation of Euler operators for
`PolyhedralBoundedSolid`, following [MANT1988] chapters 9 and 11.
*/
export class PolyhedralBoundedSolidEulerOperators {
    private static readonly FACE_MESSAGE = "Face ";
    private static readonly EDGE_MESSAGE = "Edge ";
    private static readonly NOT_FOUND_MESSAGE = " not found.";
    private static readonly EDGE_NOT_FOUND_IN_FACE_WILDCARD_MESSAGE = " - * not found in face ";
    private static readonly EDGE_NOT_FOUND_IN_FACE_MESSAGE = " not found in face ";
    private static readonly LRINGMV_MESSAGE = "lringmv";
    private static readonly ADDHE_MESSAGE = "addhe";
    private static readonly DOT = ".";

    private constructor() {}

    private static addhe(
        solid: PolyhedralBoundedSolid,
        e: _PolyhedralBoundedSolidEdge | null,
        v: _PolyhedralBoundedSolidVertex,
        where: _PolyhedralBoundedSolidHalfEdge | null,
        sign: number,
    ): _PolyhedralBoundedSolidHalfEdge | null {
        let he: _PolyhedralBoundedSolidHalfEdge;

        if (where === null) {
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                PolyhedralBoundedSolidEulerOperators.ADDHE_MESSAGE,
                "Cannot create a half-edge because the reference half-edge is null.",
            );
            return null;
        }
        if (where.parentLoop === null) {
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                PolyhedralBoundedSolidEulerOperators.ADDHE_MESSAGE,
                "Cannot create a half-edge because the reference half-edge has no parent loop.",
            );
            return null;
        }
        if (where.parentLoop.halfEdgesList === null) {
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                PolyhedralBoundedSolidEulerOperators.ADDHE_MESSAGE,
                "Cannot create a half-edge because the parent loop half-edge list is null.",
            );
            return null;
        }
        if (e === null) {
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                PolyhedralBoundedSolidEulerOperators.ADDHE_MESSAGE,
                "Cannot create a half-edge because the target edge is null.",
            );
            return null;
        }

        if (where !== null && where.parentEdge === null) {
            he = where;
        } else {
            he = new _PolyhedralBoundedSolidHalfEdge(v, where.parentLoop, solid);
            where.parentLoop.halfEdgesList.insertBefore(he, where);
            he.startingVertex = v;
        }
        he.parentEdge = e;
        he.parentLoop = where.parentLoop;

        if (sign === PolyhedralBoundedSolid.PLUS) {
            e.leftHalf = he;
        } else {
            e.rightHalf = he;
        }

        return he;
    }

    //= LOW LEVEL EULER OPERATIONS ====================================

    /**
    mvfs: MakeVertexFaceSolid.
    Operator mvfs creates a new solid representation that consist of
    one face and one vertex with coordinates specified in `p`. This
    operator has one single level of implementation (no "low level"
    or "high level versions") as other operator has.

    As described in sections [MANT1988].9.2.2, [MANT1988].11.3.1 and
    [MANT1988].11.5.1; and following the structure of sample program
    [MANT1988].11.5, this method should be used as part of every
    PolyhedralBoundedSolid constructor process, to yield to an empty
    skeletal boundary representation solid.

    Note that all correctly built solids are the result of a series of
    Euler operations over this generated skeleton, the "single skeletal
    plane model" ([MANT1988].9.2.2). The "solid" created here may not
    satisfy the intuitive notion of a solid object. Nevertheless, it is
    useful as the initial state of creating a boundary model with a sequence
    of Euler operations.
    @param solid target skeletal solid to clear.
    @param p position for the initial vertex.
    @param vertexId id assigned to the initial vertex.
    @param faceId id assigned to the initial face.
    */
    public static mvfs(solid: PolyhedralBoundedSolid, p: Vector3Dd, vertexId: number, faceId: number): void {
        if (vertexId > solid.getMaxVertexId()) solid.setMaxVertexId(vertexId);
        if (faceId > solid.getMaxFaceId()) solid.setMaxFaceId(faceId);

        const newFace = new _PolyhedralBoundedSolidFace(solid, faceId);
        const newLoop = new _PolyhedralBoundedSolidLoop(newFace);
        const newVertex = new _PolyhedralBoundedSolidVertex(solid, p, vertexId);
        const newHalfEdge = new _PolyhedralBoundedSolidHalfEdge(newVertex, newLoop, solid);
        newLoop.halfEdgesList.add(newHalfEdge);
        newLoop.boundaryStartHalfEdge = newHalfEdge;
        // The lone vertex of an empty loop points at the loop's edge-less
        // half-edge, so every vertex has a live emanating half-edge.
        newVertex.emanatingHalfEdge = newHalfEdge;
    }

    /**
    kvfs: KillVertexFaceSolid.
    Operator kvfs is the inverse of mvfs, and removes the contents of
    current solid, given that it is the skeletal solid. The solid must
    consist of a single face and vertex only.

    As described in sections [MANT1988].9.2.2 and [MANT1988].11.5.1 method
    can be used as part of final PolyhedralBoundedSolid destructor process,
    starting from an empty skeletal boundary representation solid.

    Note that all correctly built solids can be destroyed with a series of
    Euler operations over yielding to the "single skeletal plane model"
    [MANT1988].9.2.2.
    */
    public static kvfs(solid: PolyhedralBoundedSolid): void {
        if (solid.getPolygonsList().size() !== 1) {
            throw new VSDKFatalException("kvfs: Not skeletal solid, not having exactly one loop!");
        }
        if (solid.getEdgesList().size() !== 0) {
            throw new VSDKFatalException("kvfs: Not skeletal solid, having some edges!");
        }
        if (solid.getVerticesList().size() !== 1) {
            throw new VSDKFatalException("kvfs: Not skeletal solid, not having exactly one vertex !");
        }
        solid.getPolygonsList().remove(0);
        solid.getVerticesList().remove(0);
    }

    /**
    lmev: low level make edge vertex (vertex splitting operation).
    Operator lmev "splits" the vertex pointed at by `he1` and `he2`,
    and adds a new vertex and new edge between the resulting two vertices.
    The coordinates specified by `p` are assigned to the new vertex position.
    If `he1` and `he2` are the same half-edge, the operation creates the
    practical line-drawing strut needed for empty-loop construction, sweeps,
    and zero-length null edges.

    As described in sections [MANT1988].9.2.3, [MANT1988].11.3.2 and
    [MANT1988].11.5.1; and following the structure of sample program
    [MANT1988].11.6, this method has the effect of adding one new vertex
    and one new edge to the solid model.
    @param solid target solid instance.
    @param he1 first incident half-edge at the split vertex.
    @param he2 second incident half-edge at the split vertex.
    @param vertexId id assigned to the created vertex.
    @param p position for the created vertex.
    */
    public static lmev(
        solid: PolyhedralBoundedSolid,
        he1: _PolyhedralBoundedSolidHalfEdge | null,
        he2: _PolyhedralBoundedSolidHalfEdge | null,
        vertexId: number,
        p: Vector3Dd,
    ): void {
        PolyhedralBoundedSolidStatistics.recordLmevCall();
        if (he1 === null || he2 === null) {
            PolyhedralBoundedSolidStatistics.recordInvalidHalfEdgeInputCase();
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(solid, VSDK.WARNING, "lmev", "Calling with empty half-edge!");
            return;
        }
        if (he1.startingVertex !== he2.startingVertex) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            throw new VSDKFatalException("lmev: Half-edges not starting at the same vertex. Not supported case!");
        }
        if (he1 === he2) {
            PolyhedralBoundedSolidStatistics.recordHe1EqualsHe2Case();
            PolyhedralBoundedSolidEulerOperators.insertLineDrawingEdge(solid, he1, vertexId, p);
            return;
        }
        PolyhedralBoundedSolidEulerOperators.splitVertexNeighborhood(solid, he1, he2, vertexId, p);
    }

    private static splitVertexNeighborhood(
        solid: PolyhedralBoundedSolid,
        he1: _PolyhedralBoundedSolidHalfEdge,
        he2: _PolyhedralBoundedSolidHalfEdge,
        vertexId: number,
        p: Vector3Dd,
    ): void {
        let he: _PolyhedralBoundedSolidHalfEdge;

        if (vertexId > solid.getMaxVertexId()) solid.setMaxVertexId(vertexId);

        const newEdge = new _PolyhedralBoundedSolidEdge(he1.parentLoop.parentFace.parentSolid);
        const newVertex = new _PolyhedralBoundedSolidVertex(he1.parentLoop.parentFace.parentSolid, p, vertexId);

        //-----------------------------------------------------------------
        he = he1;
        while (he !== he2) {
            he.startingVertex = newVertex;
            he = he.mirrorHalfEdge()!.next()!;
        }

        //-----------------------------------------------------------------
        PolyhedralBoundedSolidEulerOperators.addhe(solid, newEdge, newVertex, he2, PolyhedralBoundedSolid.PLUS);
        PolyhedralBoundedSolidEulerOperators.addhe(
            solid,
            newEdge,
            he2.startingVertex,
            he1,
            PolyhedralBoundedSolid.MINUS,
        );

        //-----------------------------------------------------------------
        newVertex.emanatingHalfEdge = he2.previous();
        he2.startingVertex.emanatingHalfEdge = he2;
    }

    private static insertLineDrawingEdge(
        solid: PolyhedralBoundedSolid,
        he: _PolyhedralBoundedSolidHalfEdge,
        vertexId: number,
        p: Vector3Dd,
    ): void {
        if (vertexId > solid.getMaxVertexId()) solid.setMaxVertexId(vertexId);

        const newEdge = new _PolyhedralBoundedSolidEdge(he.parentLoop.parentFace.parentSolid);
        const newVertex = new _PolyhedralBoundedSolidVertex(he.parentLoop.parentFace.parentSolid, p, vertexId);
        const oldVertex = he.startingVertex;

        PolyhedralBoundedSolidEulerOperators.addhe(solid, newEdge, oldVertex, he, PolyhedralBoundedSolid.PLUS);
        PolyhedralBoundedSolidEulerOperators.addhe(solid, newEdge, newVertex, he, PolyhedralBoundedSolid.MINUS);

        newVertex.emanatingHalfEdge = he.previous();
        oldVertex.emanatingHalfEdge = he;
    }

    /**
    lkev: low level kill edge vertex (vertex joining operation).
    Operator lkev is the inverse of lmev. It removes the edge pointed at
    by `he1` and `he2`, and "joins" the two vertices `he1.startingVertex`
    and `he2.startingVertex` which must be distinct (but can, of course,
    correspond with geometrically identical points). Vertex
    `he1->startingVertex` is removed.

    As described in sections [MANT1988].9.2.3, [MANT1988].11.3.4 and
    [MANT1988].11.5.1 this method has the effect of removing one vertex
    and one edge from the solid model.

    Current implementation is not explained on [MANT1988], but leaved as
    problem [MANT1988].11.3.
    @param solid target solid instance.
    @param he1 first half-edge of the edge to remove.
    @param he2 opposite half-edge of the edge to remove.
    */
    public static lkev(
        solid: PolyhedralBoundedSolid,
        he1: _PolyhedralBoundedSolidHalfEdge | null,
        he2: _PolyhedralBoundedSolidHalfEdge | null,
    ): void {
        PolyhedralBoundedSolidStatistics.recordLkevCall();
        //-----------------------------------------------------------------
        if (he1 === null || he2 === null) {
            PolyhedralBoundedSolidStatistics.recordInvalidHalfEdgeInputCase();
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "lkev",
                "Two half-edges are needed for this Euler operator two work!",
            );
            return;
        }

        if (he1.parentEdge === null || he1.parentEdge !== he2.parentEdge) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(solid, VSDK.WARNING, "lkev", "Given half-edges must be the two halves of one edge!");
            return;
        }

        if (he1 === he2) {
            PolyhedralBoundedSolidStatistics.recordHe1EqualsHe2Case();
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(solid, VSDK.WARNING, "lkev", "Given half-edges must be different!");
            return;
        }

        //-----------------------------------------------------------------
        // Answer borrowed from [.wMANT2008]
        let he: _PolyhedralBoundedSolidHalfEdge;

        he = he2.next()!;
        while (he !== he1) {
            he.startingVertex = he2.startingVertex;
            he = he.mirrorHalfEdge()!.next()!;
        }

        // The surviving vertex must keep a live emanating half-edge
        // (defect E-05): he1.next() starts at he2's vertex; when that is he2
        // itself (strut seen from the other side) use he2.next() instead.
        let he1next: _PolyhedralBoundedSolidHalfEdge | null;
        he1next = he1.next();
        const he2next = he2.next();
        he1.parentLoop.unlistHalfEdge(he1);
        he2.parentLoop.unlistHalfEdge(he2);
        if (he1next !== he1 && he1next !== he2) {
            he2.startingVertex.emanatingHalfEdge = he1next;
        } else if (he2next !== he1 && he2next !== he2) {
            he2.startingVertex.emanatingHalfEdge = he2next;
        } else {
            // Loop becomes empty; he2 survives below as its edge-less
            // half-edge.
            he2.startingVertex.emanatingHalfEdge = he2;
        }

        solid.getEdgesList().locateWindowAtElem(he1.parentEdge!);
        solid.getEdgesList().removeElemAtWindow();
        solid.getVerticesList().locateWindowAtElem(he1.startingVertex);
        solid.getVerticesList().removeElemAtWindow();

        if (he2.parentLoop.halfEdgesList.size() <= 0) {
            he2.parentEdge = null;
            he2.parentLoop.halfEdgesList.add(he2);
            he2.parentLoop.boundaryStartHalfEdge = he2;
        }
    }

    /**
    lkef: Low Level Kill Edge Face
    Operator `lkef` is the inverse of `lmef`. It removes the edge of
    `he1` and `he2`, and "joins" the two adjacent faces by merging
    their loops. The face `he2.parentLoop.parentFace` is removed.
    `lkef` is applicable to the halves of an edge that occurs in two
    distinct faces. That is, it is assumed that:
      - `he1.parentEdge` == `he2.parentEdge`
      - `he1.parentLoop.parentFace` != `he2.parentLoop.parentFace`
    @param solid target solid instance.
    @param he1 first half-edge of the edge to remove.
    @param he2 opposite half-edge of the edge to remove.
    */
    public static lkef(
        solid: PolyhedralBoundedSolid,
        he1: _PolyhedralBoundedSolidHalfEdge,
        he2: _PolyhedralBoundedSolidHalfEdge,
    ): void {
        PolyhedralBoundedSolidStatistics.recordLkefCall();
        if (he1.parentEdge !== he2.parentEdge) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            throw new VSDKFatalException("lkef: Given half-edges must lie over the same edge. Operation aborted.");
        }
        if (he1.parentLoop.parentFace === he2.parentLoop.parentFace) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            throw new VSDKFatalException("lkef: Given half-edges must belong to different faces. Operation aborted.");
        }

        let halfEdgePivot: _PolyhedralBoundedSolidHalfEdge | null;

        halfEdgePivot = he1.next();
        const he1nextBeforeKill = halfEdgePivot;
        const he2nextBeforeKill = he2.next();
        const edgeToBeKilled = he1.parentEdge;
        const loopToBeKilled = he2.parentLoop;
        const faceToBeKilled = loopToBeKilled.parentFace;

        const migratedHalfEdges: _PolyhedralBoundedSolidHalfEdge[] = [];

        let he: _PolyhedralBoundedSolidHalfEdge | null;
        he = he2.next();
        const maxTraversal = he2.parentLoop.halfEdgesList.size() + 1;
        let traversed = 0;
        while (he !== he2) {
            if (he === null || traversed > maxTraversal) {
                PolyhedralBoundedSolidStatistics.recordConsistencyWarningCase();
                Logger.reportMessage(
                    solid,
                    VSDK.WARNING,
                    "lkef",
                    "Detected inconsistent next-chain; falling back to halfEdgesList order.",
                );
                migratedHalfEdges.length = 0;
                let idx: number;
                for (idx = 0; idx < he2.parentLoop.halfEdgesList.size(); idx++) {
                    const candidate = he2.parentLoop.halfEdgesList.get(idx)!;
                    if (candidate !== he2) {
                        migratedHalfEdges.push(candidate);
                    }
                }
                break;
            }
            migratedHalfEdges.push(he);
            he = he.next();
            traversed++;
        }

        he1.parentLoop.unlistHalfEdge(he1);
        he2.parentLoop.unlistHalfEdge(he2);
        solid.getEdgesList().locateWindowAtElem(edgeToBeKilled!);
        solid.getEdgesList().removeElemAtWindow();

        faceToBeKilled.boundariesList.locateWindowAtElem(loopToBeKilled);
        faceToBeKilled.boundariesList.removeElemAtWindow();
        solid.getPolygonsList().locateWindowAtElem(faceToBeKilled);
        solid.getPolygonsList().removeElemAtWindow();

        let i: number;
        for (i = migratedHalfEdges.length - 1; i >= 0; i--) {
            he = migratedHalfEdges[i]!;
            he.parentLoop = he1.parentLoop;
            he1.parentLoop.halfEdgesList.insertBefore(he, halfEdgePivot!);
            halfEdgePivot = he;
        }

        // If the killed face had additional loops (inner rings), transfer them
        // to the surviving face so their half-edges remain reachable.
        while (faceToBeKilled.boundariesList.size() > 0) {
            const orphanedLoop = faceToBeKilled.boundariesList.get(0)!;
            orphanedLoop.parentFace = he1.parentLoop.parentFace;
            faceToBeKilled.boundariesList.remove(0);
            he1.parentLoop.parentFace.boundariesList.add(orphanedLoop);
        }

        // Redirect vertex back-pointers away from the killed half-edges:
        // he2.next() starts at he1's vertex and he1.next() at he2's.
        if (he1.startingVertex.emanatingHalfEdge === he1 || he1.startingVertex.emanatingHalfEdge === he2) {
            he1.startingVertex.emanatingHalfEdge = he2nextBeforeKill;
        }
        if (he2.startingVertex.emanatingHalfEdge === he1 || he2.startingVertex.emanatingHalfEdge === he2) {
            he2.startingVertex.emanatingHalfEdge = he1nextBeforeKill;
        }
    }

    /**
    lmef: low level make edge face (face splitting operator).
    Operator lmef adds a new edge between `he1.startingVertex` and
    `he2.startingVertex`, and "splits" their common face into two faces
    such that `he1` will occur in the new face `newFaceId`, and `he2`
    remains in the old face. The new edge is oriented from
    `he1.startingVertex` to `he2.startingVertex`.
    Half-edges `he1` and `he2` must belong to the same loop (i.e.
    he1.parentLoop == he2.parentLoop ). They may be equal, in which case
    a "circular" face with just one edge is created. A pointer to the new
    face is returned.

    As described in sections [MANT1988].9.2.3, [MANT1988].11.3.3 and
    [MANT1988].11.5.1; and following the structure of sample program
    [MANT1988].11.7, this method is the dual to lmev. Note that creates
    the half-edges as usual, and then swaps them.
    @param solid target solid instance.
    @param he1 first half-edge used to split the loop.
    @param he2 second half-edge used to split the loop.
    @param newFaceId id assigned to the created face.
    @return created face after the split.
    */
    public static lmef(
        solid: PolyhedralBoundedSolid,
        he1: _PolyhedralBoundedSolidHalfEdge | null,
        he2: _PolyhedralBoundedSolidHalfEdge | null,
        newFaceId: number,
    ): _PolyhedralBoundedSolidFace | null {
        PolyhedralBoundedSolidStatistics.recordLmefCall();
        let he: _PolyhedralBoundedSolidHalfEdge;

        if (he1 === null) {
            PolyhedralBoundedSolidStatistics.recordInvalidHalfEdgeInputCase();
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(solid, VSDK.WARNING, "lmef", "Cannot split face because first half-edge is null.");
            return null;
        }
        if (he2 === null) {
            PolyhedralBoundedSolidStatistics.recordInvalidHalfEdgeInputCase();
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(solid, VSDK.WARNING, "lmef", "Cannot split face because second half-edge is null.");
            return null;
        }
        if (he1.parentLoop === null || he2.parentLoop === null) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "lmef",
                "Cannot split face because one input half-edge has no parent loop.",
            );
            return null;
        }
        if (he1.startingVertex === null || he2.startingVertex === null) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "lmef",
                "Cannot split face because one input half-edge has null starting vertex.",
            );
            return null;
        }
        if (he1.parentLoop !== he2.parentLoop) {
            // [MANT1988] Sec. 11.3.3 precondition (defect E-06): rejected
            // before any change to the solid.
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "lmef",
                "Cannot split face because the half-edges belong to different loops.",
            );
            return null;
        }

        if (newFaceId > solid.getMaxFaceId()) solid.setMaxFaceId(newFaceId);

        if (he1 === he2) {
            return PolyhedralBoundedSolidEulerOperators.lmefCircularFace(solid, he1, newFaceId);
        }

        const newFace = new _PolyhedralBoundedSolidFace(he1.parentLoop.parentFace.parentSolid, newFaceId);
        const oldLoop = he1.parentLoop;
        const newLoop = new _PolyhedralBoundedSolidLoop(newFace);
        const newEdge = new _PolyhedralBoundedSolidEdge(he1.parentLoop.parentFace.parentSolid);

        const migratedHalfEdges: _PolyhedralBoundedSolidHalfEdge[] = [];

        he = he1;
        while (he !== he2) {
            migratedHalfEdges.push(he);
            he = he.next()!;
            if (he === he1) break;
        }

        let i: number;
        for (i = 0; i < migratedHalfEdges.length; i++) {
            he = migratedHalfEdges[i]!;
            he.parentLoop = newLoop;
            oldLoop.unlistHalfEdge(he);
            newLoop.halfEdgesList.add(he);
        }

        const nhe1 = PolyhedralBoundedSolidEulerOperators.addhe(
            solid,
            newEdge,
            he2.startingVertex,
            he1,
            PolyhedralBoundedSolid.MINUS,
        );
        const nhe2 = PolyhedralBoundedSolidEulerOperators.addhe(
            solid,
            newEdge,
            he1.startingVertex,
            he2,
            PolyhedralBoundedSolid.PLUS,
        );
        if (nhe1 === null || nhe2 === null) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "lmef",
                "Cannot split face because one generated half-edge is null.",
            );
            return null;
        }

        newLoop.boundaryStartHalfEdge = nhe1;
        he2.parentLoop.boundaryStartHalfEdge = nhe2;

        return newFace;
    }

    /**
    `lmef(he, he)`: the "circular" face of Program 11.7 (defect E-04). The new
    edge starts and ends at `he.startingVertex`; one half lies alone in the
    new face's loop, the other is inserted before `he` in the old loop. When
    `he` is the edge-less half-edge of an empty loop it becomes the new
    face's half-edge, as `addhe` reuses it in the book.
    */
    private static lmefCircularFace(
        solid: PolyhedralBoundedSolid,
        he: _PolyhedralBoundedSolidHalfEdge,
        newFaceId: number,
    ): _PolyhedralBoundedSolidFace {
        let newFace: _PolyhedralBoundedSolidFace;
        let newLoop: _PolyhedralBoundedSolidLoop;
        let oldLoop: _PolyhedralBoundedSolidLoop;
        let newEdge: _PolyhedralBoundedSolidEdge;
        let nhe1: _PolyhedralBoundedSolidHalfEdge;
        let nhe2: _PolyhedralBoundedSolidHalfEdge;
        let v: _PolyhedralBoundedSolidVertex;

        oldLoop = he.parentLoop;
        v = he.startingVertex;
        newFace = new _PolyhedralBoundedSolidFace(solid, newFaceId);
        newLoop = new _PolyhedralBoundedSolidLoop(newFace);
        newEdge = new _PolyhedralBoundedSolidEdge(solid);

        if (he.parentEdge === null) {
            oldLoop.unlistHalfEdge(he);
            nhe1 = he;
            nhe1.parentLoop = newLoop;
            newLoop.halfEdgesList.add(nhe1);
            nhe2 = new _PolyhedralBoundedSolidHalfEdge(v, oldLoop, solid);
            oldLoop.halfEdgesList.add(nhe2);
        } else {
            nhe1 = new _PolyhedralBoundedSolidHalfEdge(v, newLoop, solid);
            newLoop.halfEdgesList.add(nhe1);
            nhe2 = new _PolyhedralBoundedSolidHalfEdge(v, oldLoop, solid);
            oldLoop.halfEdgesList.insertBefore(nhe2, he);
        }
        nhe1.parentEdge = newEdge;
        nhe2.parentEdge = newEdge;
        newEdge.rightHalf = nhe1;
        newEdge.leftHalf = nhe2;
        newLoop.boundaryStartHalfEdge = nhe1;
        oldLoop.boundaryStartHalfEdge = nhe2;
        v.emanatingHalfEdge = nhe2;
        return newFace;
    }

    /**
    lkemr: low level kill edge make ring (loop splitting operator).
    Operator lkemr removes the edge of `he1` and `he2`, and divides their
    common loop into two components (i.e., it creates a new "ring").  If
    the original loop was "outer", the component of `he1.startingVertex`
    becomes the new "outer" loop. (If this default is inappropriate, you
    can simply swap the arguments of lkemr, or use lringmv to make the
    desired loop "outer").  It is assumed that
    he1.parentEdge == he2.parentEdge and
    he1.parentLoop == he2.parentLoop.

    As described in section [MANT1988].9.2.3, the operator splits a loop
    into two new ones by removing and edge that appears twice in it. Hence,
    the operator divides a connected bounding curve of a face into two
    bounding curves, and has the net effect of removing one edge and adding
    one ring to the PolyhedralBoundedSolid data structure. The special
    cases that one or both of the resulting loops are empty are also
    included. Note that current code follows section [MANT1988].11.3.4
    and the structure of sample program [MANT1988].11.8.
    @param solid target solid instance.
    @param he1 first half-edge of the bridge edge.
    @param he2 opposite half-edge of the bridge edge.
    */
    public static lkemr(
        solid: PolyhedralBoundedSolid,
        he1: _PolyhedralBoundedSolidHalfEdge | null,
        he2: _PolyhedralBoundedSolidHalfEdge | null,
    ): void {
        PolyhedralBoundedSolidStatistics.recordLkemrCall();
        //-----------------------------------------------------------------
        if (
            he1 === null ||
            he2 === null ||
            he1 === he2 ||
            he1.parentEdge === null ||
            he1.parentEdge !== he2.parentEdge ||
            he1.parentLoop !== he2.parentLoop
        ) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "lkemr",
                "Given half-edges must be the two halves of one edge in one loop. " + "Operation aborted.",
            );
            return;
        }

        //-----------------------------------------------------------------
        // Following Program 11.8: the loop he1, R, he2, O (R and O being the
        // half-edges strictly between he1 and he2, and between he2 and he1)
        // becomes the old loop {he1, O} and the new ring {he2, R}; then the
        // two halves of the bridge are deleted. A component that ends up
        // without half-edges keeps its bridge half as the edge-less
        // half-edge of an empty loop (the book's delhe), so both "one
        // resulting loop empty" cases of Sec. 11.3.4 are handled (defect
        // E-02). Every vertex keeps a live emanating half-edge (E-03).
        let he: _PolyhedralBoundedSolidHalfEdge;
        let newLoop: _PolyhedralBoundedSolidLoop;
        let oldLoop: _PolyhedralBoundedSolidLoop;
        let killedEdge: _PolyhedralBoundedSolidEdge | null;
        let ring: _PolyhedralBoundedSolidHalfEdge[];
        let i: number;

        oldLoop = he1.parentLoop;
        killedEdge = he1.parentEdge;
        ring = [];
        for (he = he1.next()!; he !== he2; he = he.next()!) {
            ring.push(he);
        }
        newLoop = new _PolyhedralBoundedSolidLoop(oldLoop.parentFace);

        for (i = 0; i < ring.length; i++) {
            he = ring[i]!;
            oldLoop.halfEdgesList.locateWindowAtElem(he);
            oldLoop.halfEdgesList.removeElemAtWindow();
            newLoop.halfEdgesList.add(he);
            he.parentLoop = newLoop;
        }

        // he2 always leaves the old loop.
        oldLoop.halfEdgesList.locateWindowAtElem(he2);
        oldLoop.halfEdgesList.removeElemAtWindow();
        if (ring.length === 0) {
            he2.parentEdge = null;
            he2.parentLoop = newLoop;
            newLoop.halfEdgesList.add(he2);
        }
        newLoop.boundaryStartHalfEdge = newLoop.halfEdgesList.get(0);
        he2.startingVertex.emanatingHalfEdge = newLoop.boundaryStartHalfEdge;

        if (oldLoop.halfEdgesList.size() <= 1) {
            // Only he1 is left: it becomes the edge-less half-edge.
            he1.parentEdge = null;
            oldLoop.boundaryStartHalfEdge = he1;
            he1.startingVertex.emanatingHalfEdge = he1;
        } else {
            const he1next = he1.next();
            oldLoop.halfEdgesList.locateWindowAtElem(he1);
            oldLoop.halfEdgesList.removeElemAtWindow();
            oldLoop.boundaryStartHalfEdge = he1next;
            he1.startingVertex.emanatingHalfEdge = he1next;
        }

        solid.getEdgesList().locateWindowAtElem(killedEdge!);
        solid.getEdgesList().removeElemAtWindow();
    }

    /**
    lkfmrh: low level kill face make ring hole in same shell.
    Operator `lkfmrh` merges two faces `face1` and `face2` by making
    the loop of the latter a ring into the former. Face `face2` is hence
    removed.
    PRE: It is assumed that `face2` is simple, i.e., has just one loop.
    This method implements the low-level operator family from exercise
    [MANT1988].11.5, following the functional definition of section
    [MANT1988].11.5.1 for the same-shell case.
    @param solid target solid instance.
    @param face1 destination face that receives the ring.
    @param face2 source face whose loop becomes a ring.
    */
    public static lkfmrh(
        solid: PolyhedralBoundedSolid,
        face1: _PolyhedralBoundedSolidFace | null,
        face2: _PolyhedralBoundedSolidFace | null,
    ): void {
        if (
            face1 === null ||
            face2 === null ||
            face1 === face2 ||
            face1.parentSolid !== solid ||
            face2.parentSolid !== solid
        ) {
            Logger.reportMessage(solid, VSDK.WARNING, "lkfmrh", "Faces must be two distinct faces of the given solid!");
            return;
        }
        if (face2.boundariesList.size() > 1) {
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "lkfmrh",
                "Internal face to form new loop must have just one boundary!",
            );
            return;
        }

        let he: _PolyhedralBoundedSolidHalfEdge;
        let i: number;

        const oldLoop = face2.boundariesList.get(0)!;
        const newLoop = new _PolyhedralBoundedSolidLoop(face1);

        for (i = 0; i < oldLoop.halfEdgesList.size(); i++) {
            he = oldLoop.halfEdgesList.get(i)!;
            he.parentLoop = newLoop;
            newLoop.halfEdgesList.add(he);
        }
        newLoop.boundaryStartHalfEdge = newLoop.halfEdgesList.get(0);

        solid.getPolygonsList().locateWindowAtElem(face2);
        solid.getPolygonsList().removeElemAtWindow();
    }

    /**
    lmfkrh: low level make face kill ring hole.
    Operator `lmfkrh` is the inverse of `lkfmrh`. It makes the loop `l`
    the outer loop of a new face `newFaceId`. It is assumed that `l` is an
    inner loop of its parent face.
    This method implements the inverse operator of the low-level family from
    exercise [MANT1988].11.5, following section [MANT1988].11.5.1.
    @param solid target solid instance.
    @param l inner loop to promote as outer loop of a new face.
    @param newFaceId id assigned to the created face.
    @return created face containing the promoted loop.
    */
    public static lmfkrh(
        solid: PolyhedralBoundedSolid,
        l: _PolyhedralBoundedSolidLoop,
        newFaceId: number,
    ): _PolyhedralBoundedSolidFace {
        const newFace = new _PolyhedralBoundedSolidFace(solid, newFaceId);

        if (newFaceId > solid.getMaxFaceId()) solid.setMaxFaceId(newFaceId);

        l.parentFace.boundariesList.locateWindowAtElem(l);
        l.parentFace.boundariesList.removeElemAtWindow();
        l.parentFace = newFace;
        newFace.boundariesList.add(l);

        return newFace;
    }

    /**
    lkimrh: low level kill inner make ring hole.
    Naming alias for the [MANT1988].11.5 low-level operator family.
    Current Vitral codebase uses the `lkfmrh` name for the same semantics.
    @param solid target solid instance.
    @param face1 destination face that receives the ring.
    @param face2 source face whose loop becomes a ring.
    */
    public static lkimrh(
        solid: PolyhedralBoundedSolid,
        face1: _PolyhedralBoundedSolidFace,
        face2: _PolyhedralBoundedSolidFace,
    ): void {
        PolyhedralBoundedSolidEulerOperators.lkfmrh(solid, face1, face2);
    }

    /**
    lmikrh: low level make inner kill ring hole.
    Naming alias for the [MANT1988].11.5 low-level inverse operator family.
    Current Vitral codebase uses the `lmfkrh` name for the same semantics.
    @param solid target solid instance.
    @param l inner loop to promote as outer loop of a new face.
    @param newFaceId id assigned to the created face.
    @return created face containing the promoted loop.
    */
    public static lmikrh(
        solid: PolyhedralBoundedSolid,
        l: _PolyhedralBoundedSolidLoop,
        newFaceId: number,
    ): _PolyhedralBoundedSolidFace {
        return PolyhedralBoundedSolidEulerOperators.lmfkrh(solid, l, newFaceId);
    }

    private static failLringmv(solid: PolyhedralBoundedSolid, message: string): boolean {
        PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
        Logger.reportMessage(solid, VSDK.WARNING, PolyhedralBoundedSolidEulerOperators.LRINGMV_MESSAGE, message);
        return false;
    }

    private static validateLringmvInput(
        solid: PolyhedralBoundedSolid,
        loop: _PolyhedralBoundedSolidLoop | null,
        destinationFace: _PolyhedralBoundedSolidFace | null,
    ): _PolyhedralBoundedSolidFace | null {
        if (loop === null || destinationFace === null) {
            PolyhedralBoundedSolidEulerOperators.failLringmv(solid, "Null input loop or destination face.");
            return null;
        }

        const sourceFace = loop.parentFace;
        if (sourceFace === null) {
            PolyhedralBoundedSolidEulerOperators.failLringmv(solid, "Given loop does not have a parent face.");
            return null;
        }
        if (sourceFace.parentSolid !== destinationFace.parentSolid) {
            PolyhedralBoundedSolidEulerOperators.failLringmv(
                solid,
                "Loop move across different solids is not supported.",
            );
            return null;
        }
        return sourceFace;
    }

    private static reorderLoopInSameFace(
        solid: PolyhedralBoundedSolid,
        loop: _PolyhedralBoundedSolidLoop,
        face: _PolyhedralBoundedSolidFace,
        setAsOuterLoop: boolean,
    ): boolean {
        if (!face.boundariesList.locateWindowAtElem(loop)) {
            return PolyhedralBoundedSolidEulerOperators.failLringmv(
                solid,
                "Given loop was not found in its parent face boundaries.",
            );
        }
        if (setAsOuterLoop) {
            PolyhedralBoundedSolidEulerOperators.promoteLoopAsOuter(face, loop);
            return true;
        }
        return PolyhedralBoundedSolidEulerOperators.demoteLoopAsInner(solid, face, loop);
    }

    private static promoteLoopAsOuter(face: _PolyhedralBoundedSolidFace, loop: _PolyhedralBoundedSolidLoop): void {
        const numberOfLoops = face.boundariesList.size();
        if (numberOfLoops <= 0 || face.boundariesList.get(0) === loop) {
            return;
        }
        face.boundariesList.swapElements(loop, face.boundariesList.get(0)!);
    }

    private static demoteLoopAsInner(
        solid: PolyhedralBoundedSolid,
        face: _PolyhedralBoundedSolidFace,
        loop: _PolyhedralBoundedSolidLoop,
    ): boolean {
        const numberOfLoops = face.boundariesList.size();
        if (numberOfLoops <= 1) {
            return PolyhedralBoundedSolidEulerOperators.failLringmv(
                solid,
                "Cannot mark the only boundary loop as inner.",
            );
        }
        if (face.boundariesList.get(0) !== loop) {
            return true;
        }
        face.boundariesList.swapElements(loop, face.boundariesList.get(1)!);
        return true;
    }

    private static moveLoopAcrossFaces(
        solid: PolyhedralBoundedSolid,
        loop: _PolyhedralBoundedSolidLoop,
        sourceFace: _PolyhedralBoundedSolidFace,
        destinationFace: _PolyhedralBoundedSolidFace,
        setAsOuterLoop: boolean,
    ): boolean {
        if (sourceFace.boundariesList.size() <= 1) {
            return PolyhedralBoundedSolidEulerOperators.failLringmv(
                solid,
                "Cannot move the only boundary loop of a face.",
            );
        }
        if (!setAsOuterLoop && destinationFace.boundariesList.size() <= 0) {
            return PolyhedralBoundedSolidEulerOperators.failLringmv(
                solid,
                "Cannot insert an inner loop into a face without outer loop.",
            );
        }
        if (!sourceFace.boundariesList.locateWindowAtElem(loop)) {
            return PolyhedralBoundedSolidEulerOperators.failLringmv(
                solid,
                "Given loop was not found in source face boundaries.",
            );
        }

        sourceFace.boundariesList.removeElemAtWindow();
        loop.parentFace = destinationFace;

        if (setAsOuterLoop) {
            destinationFace.boundariesList.push(loop);
        } else {
            destinationFace.boundariesList.add(loop);
        }
        return true;
    }

    /**
    Method `lringmv` moves the loop `l` from its parent face to face `toFace`.
    If `setAsOuterLoop` is false, `l` becomes an "inner" loop of `toFace`.
    Otherwise, `l` is positioned as the outer loop (first boundary) of
    `toFace`.

    If `l.parentFace` == `toFace`, no topological move is performed; only loop
    ordering is adjusted to satisfy the requested inner/outer role.

    Current implementation validates non-null arguments, same-solid movement,
    source face non-emptiness after move, and that an inner loop is not added
    to a face without an outer loop.

    Note that `lringmv` is an addendum to `lmef`, and not an Euler operator.
    It is part of the practical support required by the operator family of
    exercise [MANT1988].11.5, and follows section [MANT1988].11.5.1.
    @param solid target solid instance.
    @param l loop to move or reorder.
    @param toFace destination face.
    @param setAsOuterLoop true to place `l` as outer loop; false for inner loop.
    @return true if the move/reorder succeeds; false otherwise.
    */
    public static lringmv(
        solid: PolyhedralBoundedSolid,
        l: _PolyhedralBoundedSolidLoop | null,
        toFace: _PolyhedralBoundedSolidFace | null,
        setAsOuterLoop: boolean,
    ): boolean {
        PolyhedralBoundedSolidStatistics.recordLringmvCall();
        const fromFace = PolyhedralBoundedSolidEulerOperators.validateLringmvInput(solid, l, toFace);
        if (fromFace === null) {
            return false;
        }
        if (fromFace === toFace) {
            return PolyhedralBoundedSolidEulerOperators.reorderLoopInSameFace(solid, l!, toFace, setAsOuterLoop);
        }
        return PolyhedralBoundedSolidEulerOperators.moveLoopAcrossFaces(solid, l!, fromFace, toFace!, setAsOuterLoop);
    }

    /**
    lmekr: low level make edge kill ring.
    Operator `lmekr` is the inverse of `lkemr`. It inserts a new edge
    between the starting vertices of `he1` and `he2`, and merges the
    corresponding loops into one loop (i.e. removes a ring). The
    operator assumes that:
      - `he1.parentLoop` and `he2.parentLoop` are different, i.e.
        `he1` and `he2` belong to two distinct loops.
      - `he1.parentLoop.parentFace` and `he2.parentLoop.parentFace`
        are equal, i.e. they occur in a single face.
    Current implementation was programmed as an answer to exercise
    [MANT1988].11.4, and follows the signature from section
    [MANT1988].11.5.1.
    @param solid target solid instance.
    @param he1 half-edge on the loop to merge.
    @param he2 half-edge on the ring to merge.
    */
    public static lmekr(
        solid: PolyhedralBoundedSolid,
        he1: _PolyhedralBoundedSolidHalfEdge,
        he2: _PolyhedralBoundedSolidHalfEdge,
    ): void {
        PolyhedralBoundedSolidStatistics.recordLmekrCall();
        //-----------------------------------------------------------------
        if (he1.parentLoop === he2.parentLoop) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "lmekr",
                "Given half-edges are on the same loop. Operation aborted.",
            );
            return;
        }
        if (he1.parentLoop.parentFace !== he2.parentLoop.parentFace) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "lmekr",
                "Given half-edges are not on the same face. Operation aborted.",
            );
            return;
        }

        //-----------------------------------------------------------------
        let he: _PolyhedralBoundedSolidHalfEdge;

        const migratedHalfEdges: _PolyhedralBoundedSolidHalfEdge[] = [];
        const ringToKill = he2.parentLoop;

        he = he2;
        do {
            migratedHalfEdges.push(he);
            he = he.next()!;
        } while (he !== he2);

        //-----------------------------------------------------------------
        let i: number;

        // The loop of `he2` is the one removed. When it is the outer loop of
        // the face (`he1` is on a ring and `he2` on the outer boundary), the
        // merged loop is the outer boundary from now on and must take its
        // place. It used to stay where the ring of `he1` was, and whatever
        // other ring came first in the list was taken for the outer loop
        // (plan-stage11 Phase F).
        const killsOuterLoop = ringToKill.parentFace.boundariesList.get(0) === ringToKill;

        ringToKill.halfEdgesList.clear();
        ringToKill.parentFace.boundariesList.locateWindowAtElem(ringToKill);
        ringToKill.parentFace.boundariesList.removeElemAtWindow();
        if (killsOuterLoop) {
            PolyhedralBoundedSolidEulerOperators.promoteLoopAsOuter(he1.parentLoop.parentFace, he1.parentLoop);
        }

        //-----------------------------------------------------------------
        const v1 = he1.startingVertex;
        const v2 = he2.startingVertex;

        const newEdge = new _PolyhedralBoundedSolidEdge(he1.parentLoop.parentFace.parentSolid);

        const heLast = PolyhedralBoundedSolidEulerOperators.addhe(
            solid,
            newEdge,
            v2,
            he1,
            PolyhedralBoundedSolid.MINUS,
        );
        newEdge.rightHalf = PolyhedralBoundedSolidEulerOperators.addhe(
            solid,
            newEdge,
            v1,
            heLast,
            PolyhedralBoundedSolid.MINUS,
        );
        newEdge.leftHalf = heLast;

        // Alas! This rare condition of not adding a migrated half edges
        // list of size 1 is to avoid adding an 0-length half-edge with no
        // parent edge and no mirror edge when loop is of size 1 vertex.
        for (i = 0; i < migratedHalfEdges.length && migratedHalfEdges.length > 1; i++) {
            he = migratedHalfEdges[i]!;
            he.parentLoop = he1.parentLoop;
            he1.parentLoop.halfEdgesList.insertBefore(he, heLast!);
        }
        // heLast starts at v2 and is always in the merged loop; a lone ring
        // vertex had its edge-less half-edge dropped above (defect E-03).
        v2.emanatingHalfEdge = heLast;
    }

    //= HIGH LEVEL EULER OPERATIONS ===================================

    /**
    smev: "Strut" or line-drawing "Simplified" version of mev operator.
    See mev method for a complete description.
    @param solid target solid instance.
    @param f1 face id where the strut is created.
    @param v1 start vertex id for the new strut.
    @param v4 id assigned to the created vertex.
    @param p position for the created vertex.
    @return true if the operation succeeds; false otherwise.
    */
    public static smev(solid: PolyhedralBoundedSolid, f1: number, v1: number, v4: number, p: Vector3Dd): boolean {
        const oldFace1 = solid.findFace(f1);
        if (oldFace1 === null) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "mev",
                PolyhedralBoundedSolidEulerOperators.FACE_MESSAGE +
                    f1 +
                    PolyhedralBoundedSolidEulerOperators.NOT_FOUND_MESSAGE,
            );
            return false;
        }
        const he1 = oldFace1.findHalfEdge(v1);
        if (he1 === null) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "mev",
                PolyhedralBoundedSolidEulerOperators.EDGE_MESSAGE +
                    v1 +
                    PolyhedralBoundedSolidEulerOperators.EDGE_NOT_FOUND_IN_FACE_WILDCARD_MESSAGE +
                    f1 +
                    PolyhedralBoundedSolidEulerOperators.DOT,
            );
            return false;
        }
        PolyhedralBoundedSolidEulerOperators.lmev(solid, he1, he1, v4, p);
        return true;
    }

    /**
    mev: (high level version) make edge vertex (vertex splitting operation).
    Operator `mev` divides the cycle of edges around the vertex `v1` so
    that all edges from `v1` -> `v2` (inclusive) to `v1` -> `v3` (exclusive)
    will become adjacent to a new vertex `v4`.  The vertices `v1` and `v4`
    are joined with a new edge.  Coordinates defined in `p` are assigned
    to `v4`.
    Similarly to the corresponding low level operator `lmev`, various
    special cases are possible.  Of particular use is the "line-drawing"
    case that `f1` = `f2` and `v2` = `v3`.  In this case, a new edge to a
    new vertex will be added within the face. We shall call such "dangling"
    edges <I>struts</I>.  This case occurs so frequently that it is included
    a "convenience" procedure `smev` that performs this operation.  The
    procedure assumes that `v1` appears just once in `f1`; hence, the
    argument `v2` can be left out.
    @param solid target solid instance.
    @param f1 first face id used to locate `v1 -> v2`.
    @param f2 second face id used to locate `v1 -> v3`.
    @param v1 shared source vertex id.
    @param v2 target vertex id for the first half-edge.
    @param v3 target vertex id for the second half-edge.
    @param v4 id assigned to the created vertex.
    @param p position for the created vertex.
    @return true if the operation succeeds; false otherwise.
    */
    public static mev(
        solid: PolyhedralBoundedSolid,
        f1: number,
        f2: number,
        v1: number,
        v2: number,
        v3: number,
        v4: number,
        p: Vector3Dd,
    ): boolean {
        const oldFace1 = solid.findFace(f1);
        if (oldFace1 === null) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "mev",
                PolyhedralBoundedSolidEulerOperators.FACE_MESSAGE +
                    f1 +
                    PolyhedralBoundedSolidEulerOperators.NOT_FOUND_MESSAGE,
            );
            return false;
        }
        const oldFace2 = solid.findFace(f2);
        if (oldFace2 === null) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "mev",
                PolyhedralBoundedSolidEulerOperators.FACE_MESSAGE +
                    f2 +
                    PolyhedralBoundedSolidEulerOperators.NOT_FOUND_MESSAGE,
            );
            return false;
        }
        const he1 = oldFace1.findHalfEdge(v1, v2);
        if (he1 === null) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "mev",
                PolyhedralBoundedSolidEulerOperators.EDGE_MESSAGE +
                    v1 +
                    " - " +
                    v2 +
                    PolyhedralBoundedSolidEulerOperators.EDGE_NOT_FOUND_IN_FACE_MESSAGE +
                    f1 +
                    PolyhedralBoundedSolidEulerOperators.DOT,
            );
            return false;
        }
        const he2 = oldFace2.findHalfEdge(v1, v3);
        if (he2 === null) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "mev",
                PolyhedralBoundedSolidEulerOperators.EDGE_MESSAGE +
                    v1 +
                    " - " +
                    v3 +
                    PolyhedralBoundedSolidEulerOperators.EDGE_NOT_FOUND_IN_FACE_MESSAGE +
                    f2 +
                    PolyhedralBoundedSolidEulerOperators.DOT,
            );
            return false;
        }
        PolyhedralBoundedSolidEulerOperators.lmev(solid, he1, he2, v4, p);
        return true;
    }

    /**
    smef: simplified version of mef operator.
    See mef method for a complete description.
    @param solid target solid instance.
    @param f1 face id that contains both reference vertices.
    @param v1 first vertex id used to find the first half-edge.
    @param v3 second vertex id used to find the second half-edge.
    @param newFaceId id assigned to the created face.
    @return true if the operation succeeds; false otherwise.
    */
    public static smef(solid: PolyhedralBoundedSolid, f1: number, v1: number, v3: number, newFaceId: number): boolean {
        const oldFace1 = solid.findFace(f1);
        if (oldFace1 === null) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "smef",
                PolyhedralBoundedSolidEulerOperators.FACE_MESSAGE +
                    f1 +
                    PolyhedralBoundedSolidEulerOperators.NOT_FOUND_MESSAGE,
            );
            return false;
        }
        const he1 = oldFace1.findHalfEdge(v1);
        if (he1 === null) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "smef",
                PolyhedralBoundedSolidEulerOperators.EDGE_MESSAGE +
                    v1 +
                    PolyhedralBoundedSolidEulerOperators.EDGE_NOT_FOUND_IN_FACE_WILDCARD_MESSAGE +
                    f1 +
                    PolyhedralBoundedSolidEulerOperators.DOT,
            );
            return false;
        }
        const he2 = oldFace1.findHalfEdge(v3);
        if (he2 === null) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "smef",
                PolyhedralBoundedSolidEulerOperators.EDGE_MESSAGE +
                    v3 +
                    PolyhedralBoundedSolidEulerOperators.EDGE_NOT_FOUND_IN_FACE_WILDCARD_MESSAGE +
                    f1 +
                    PolyhedralBoundedSolidEulerOperators.DOT,
            );
            return false;
        }
        PolyhedralBoundedSolidEulerOperators.lmef(solid, he1, he2, newFaceId);
        return true;
    }

    /**
    mef: (high level version) make edge face (face splitting operation).
    Operator `mef` connects the vertices `v1` and `v3` of face `f1` with
    a new edge, and creates a new face `f2`. Similarly to method `smev`,
    there is included a convenience procedure `smef` that leaves the arguments
    `v1` and `v4` out; that method should be applied only if `v1` and `v3`
    are known to occur just once in the face.
    Executes a `lmef` in half-edges v1-v2, v3-v4 in respective faces `f1`
    and `f2`, and assigns to the new face the `newFaceId`.
    @param solid target solid instance.
    @param f1 first face id used to locate `v1 -> v2`.
    @param f2 second face id used to locate `v3 -> v4`.
    @param v1 first edge source vertex id.
    @param v2 first edge target vertex id.
    @param v3 second edge source vertex id.
    @param v4 second edge target vertex id.
    @param newFaceId id assigned to the created face.
    @return true if the operation succeeds; false otherwise.
    */
    public static mef(
        solid: PolyhedralBoundedSolid,
        f1: number,
        f2: number,
        v1: number,
        v2: number,
        v3: number,
        v4: number,
        newFaceId: number,
    ): boolean {
        const oldFace1 = solid.findFace(f1);
        if (oldFace1 === null) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "mef",
                PolyhedralBoundedSolidEulerOperators.FACE_MESSAGE +
                    f1 +
                    PolyhedralBoundedSolidEulerOperators.NOT_FOUND_MESSAGE,
            );
            return false;
        }
        const oldFace2 = solid.findFace(f2);
        if (oldFace2 === null) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "mef",
                PolyhedralBoundedSolidEulerOperators.FACE_MESSAGE +
                    f2 +
                    PolyhedralBoundedSolidEulerOperators.NOT_FOUND_MESSAGE,
            );
            return false;
        }
        const he1 = oldFace1.findHalfEdge(v1, v2);
        if (he1 === null) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "mef",
                PolyhedralBoundedSolidEulerOperators.EDGE_MESSAGE +
                    v1 +
                    " - " +
                    v2 +
                    PolyhedralBoundedSolidEulerOperators.EDGE_NOT_FOUND_IN_FACE_MESSAGE +
                    f1 +
                    PolyhedralBoundedSolidEulerOperators.DOT,
            );
            return false;
        }
        const he2 = oldFace2.findHalfEdge(v3, v4);
        if (he2 === null) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "mef",
                PolyhedralBoundedSolidEulerOperators.EDGE_MESSAGE +
                    v3 +
                    " - " +
                    v4 +
                    PolyhedralBoundedSolidEulerOperators.EDGE_NOT_FOUND_IN_FACE_MESSAGE +
                    f2 +
                    PolyhedralBoundedSolidEulerOperators.DOT,
            );
            return false;
        }
        PolyhedralBoundedSolidEulerOperators.lmef(solid, he1, he2, newFaceId);
        return true;
    }

    /**
    kemr: (high level version) kill edge make ring (loop splitting operation).
    Executes a `lkemr` in half-edges v1-v2, v3-v4 in respective faces `f1`
    and `f2`.
    @param solid target solid instance.
    @param f1 first face id used to locate `v1 -> v2`.
    @param f2 second face id used to locate `v3 -> v4`.
    @param v1 first edge source vertex id.
    @param v2 first edge target vertex id.
    @param v3 second edge source vertex id.
    @param v4 second edge target vertex id.
    @return true if the operation succeeds; false otherwise.
    */
    public static kemr(
        solid: PolyhedralBoundedSolid,
        f1: number,
        f2: number,
        v1: number,
        v2: number,
        v3: number,
        v4: number,
    ): boolean {
        const oldFace1 = solid.findFace(f1);
        if (oldFace1 === null) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "kemr",
                PolyhedralBoundedSolidEulerOperators.FACE_MESSAGE +
                    f1 +
                    PolyhedralBoundedSolidEulerOperators.NOT_FOUND_MESSAGE,
            );
            return false;
        }
        const oldFace2 = solid.findFace(f2);
        if (oldFace2 === null) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "kemr",
                PolyhedralBoundedSolidEulerOperators.FACE_MESSAGE +
                    f2 +
                    PolyhedralBoundedSolidEulerOperators.NOT_FOUND_MESSAGE,
            );
            return false;
        }
        const he1 = oldFace1.findHalfEdge(v1, v2);
        if (he1 === null) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "kemr",
                PolyhedralBoundedSolidEulerOperators.EDGE_MESSAGE +
                    v1 +
                    " - " +
                    v2 +
                    PolyhedralBoundedSolidEulerOperators.EDGE_NOT_FOUND_IN_FACE_MESSAGE +
                    f1 +
                    PolyhedralBoundedSolidEulerOperators.DOT,
            );
            return false;
        }
        const he2 = oldFace2.findHalfEdge(v3, v4);
        if (he2 === null) {
            PolyhedralBoundedSolidStatistics.recordOperationFailureCase();
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "kemr",
                PolyhedralBoundedSolidEulerOperators.EDGE_MESSAGE +
                    v3 +
                    " - " +
                    v4 +
                    PolyhedralBoundedSolidEulerOperators.EDGE_NOT_FOUND_IN_FACE_MESSAGE +
                    f2 +
                    PolyhedralBoundedSolidEulerOperators.DOT,
            );
            return false;
        }
        PolyhedralBoundedSolidEulerOperators.lkemr(solid, he1, he2);
        return true;
    }

    /**
    kfmrh: (high level version) KillFaceMakeRingHole
    (connected sum topological operation, global manipulation).
    Operator `kfmrhSameShell` "merges" two faces `f1` and `f2` by making
    the latter an interior loop of the former. Face `f2` is removed.
    As noted in section [MANT1988].9.2.4, the name `kfmrh` is actually a
    misnomer, because the operator does not necessarily create a "hole".
    Actually, `kfmrh` creates a hole only if the two argument faces belong
    to the same shell.  This method implements that case, taking `this`
    solid as the only shell.
    This method is the high-level counterpart used in the [MANT1988].11.5
    operator family implementation for the same-shell case.
    @param solid target solid instance.
    @param f1 destination face id.
    @param f2 source face id to be merged as ring.
    @return true if the operation succeeds; false otherwise.
    */
    public static kfmrh(solid: PolyhedralBoundedSolid, f1: number, f2: number): boolean {
        const oldFace1 = solid.findFace(f1);
        if (oldFace1 === null) {
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "kfmrh",
                PolyhedralBoundedSolidEulerOperators.FACE_MESSAGE +
                    f1 +
                    PolyhedralBoundedSolidEulerOperators.NOT_FOUND_MESSAGE,
            );
            return false;
        }
        const oldFace2 = solid.findFace(f2);
        if (oldFace2 === null) {
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "kfmrh",
                PolyhedralBoundedSolidEulerOperators.FACE_MESSAGE +
                    f2 +
                    PolyhedralBoundedSolidEulerOperators.NOT_FOUND_MESSAGE,
            );
            return false;
        }
        PolyhedralBoundedSolidEulerOperators.lkfmrh(solid, oldFace1, oldFace2);
        return true;
    }
}
