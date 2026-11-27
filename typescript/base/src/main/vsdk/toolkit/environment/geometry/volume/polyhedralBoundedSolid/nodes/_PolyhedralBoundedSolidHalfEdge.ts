//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

import { FundamentalEntity } from "../../../../../common/FundamentalEntity.js";
import type { ArrayLinkable } from "../../../../../common/dataStructures/ArrayLinkable.js";
import { Vector3Dd } from "../../../../../common/linealAlgebra/Vector3Dd.js";
import type { PolyhedralBoundedSolid } from "../PolyhedralBoundedSolid.js";
import type { _PolyhedralBoundedSolidEdge } from "./_PolyhedralBoundedSolidEdge.js";
import type { _PolyhedralBoundedSolidLoop } from "./_PolyhedralBoundedSolidLoop.js";
import type { _PolyhedralBoundedSolidVertex } from "./_PolyhedralBoundedSolidVertex.js";

/**
As noted in [MANT1988].10.2.1, a `_PolyhedralBoundedSolidHalfEdge` describes
one line segment inside a `_PolyhedralBoundedSolidLoop`. It has only a
reference to a vertex in a `PolyhedralBoundedSolid`.

Note that in the sake of simplify and eficiency current programming
implementation this class exhibit public access attributes. It is important
to note that those attributes will only be accessed directly from related
classes in the same package
(vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes) and
from methods in the `PolyhedralBoundedSolid` class, and that they should
not be used from outer classes.
*/
export class _PolyhedralBoundedSolidHalfEdge extends FundamentalEntity implements ArrayLinkable {
    public static readonly LEFT_SIDE = 1;
    public static readonly RIGHT_SIDE = 2;
    public static readonly NO_SIDE = 3;

    /// Defined as presented in [MANT1988].10.2.1
    public parentLoop: _PolyhedralBoundedSolidLoop;

    /// Defined as presented in [MANT1988].10.2.2. Note that as commented in
    /// [MANT1988].10.2.2, this reference can be `null` in the special case
    /// of empty loops.
    public parentEdge: _PolyhedralBoundedSolidEdge | null;

    /// Defined as presented in [MANT1988].10.2.1
    public startingVertex: _PolyhedralBoundedSolidVertex;

    //
    public id: number;
    private static currentId = 1;

    /// Place of this half-edge in `parentLoop.halfEdgesList`. It plays the
    /// role of the intrusive `nxt`/`prv` links of [MANT1988] Program 10.1:
    /// `next()` and `previous()` are O(1) through it. The list keeps it up
    /// to date (see `ArrayLinkable`).
    private listOwner: object | null = null;
    private listIndex = -1;

    public getListOwner(): object | null {
        return this.listOwner;
    }

    public getListIndex(): number {
        return this.listIndex;
    }

    public setListSlot(owner: object | null, index: number): void {
        this.listOwner = owner;
        this.listIndex = index;
    }

    //=================================================================
    public constructor(
        v: _PolyhedralBoundedSolidVertex,
        parentLoop: _PolyhedralBoundedSolidLoop,
        _parentSolid: PolyhedralBoundedSolid,
    ) {
        super();
        this.startingVertex = v;
        this.parentLoop = parentLoop;
        this.parentEdge = null;

        this.id = _PolyhedralBoundedSolidHalfEdge.currentId;
        _PolyhedralBoundedSolidHalfEdge.currentId++;
    }

    /**
    Locates the previous half edge in current list
    @return requested half edge
    */
    public previous(): _PolyhedralBoundedSolidHalfEdge | null {
        return this.parentLoop.halfEdgesList.previousOf(this);
    }

    /**
    Locates the next half edge in current list
    @return requested halfedge
    */
    public next(): _PolyhedralBoundedSolidHalfEdge | null {
        return this.parentLoop.halfEdgesList.nextOf(this);
    }

    private determineSideness(): number {
        if (this.parentEdge === null) {
            return _PolyhedralBoundedSolidHalfEdge.NO_SIDE;
        }
        if (this === this.parentEdge.leftHalf) {
            return _PolyhedralBoundedSolidHalfEdge.LEFT_SIDE;
        } else if (this === this.parentEdge.rightHalf) {
            return _PolyhedralBoundedSolidHalfEdge.RIGHT_SIDE;
        }
        return _PolyhedralBoundedSolidHalfEdge.NO_SIDE;
    }

    /**
    Given current half edge, this method returns complementary half edge
    with respect to parent edge. Note that this code corresponds to macro
    `mate(he)`, defined in program [MANT1988] 10.2, and annotated in
    sections [MANT1988].10.3. and [MANT1988].10.4.2.
    @return complementary halfedge
    */
    public mirrorHalfEdge(): _PolyhedralBoundedSolidHalfEdge | null {
        if (this.parentEdge === null) return null;

        if (this === this.parentEdge.rightHalf) {
            return this.parentEdge.leftHalf;
        }
        return this.parentEdge.rightHalf;
    }

    /**
    Given `this` and `other` halfedges, returns true if their starting vertexes
    position are nearly equal (with respect to a the given `tolerance`).
    This method follows the suggested funcionality of procedure "match" from
    program [MANT1988].12.9. and presented in section [MANT1988].12.4.2.
    @param other
    @param tolerance
    @return true if given halfedges are very close (under tolerance), false
    if they are far away
    */
    public vertexPositionMatch(other: _PolyhedralBoundedSolidHalfEdge, tolerance: number): boolean {
        return Vector3Dd.distance(this.startingVertex.position, other.startingVertex.position) <= tolerance;
    }

    public override toString(): string {
        let msg: string;
        msg = "HalfEdge id " + this.id + ". ";

        msg = msg + "From vertex [" + this.startingVertex.id + "] ";
        msg = msg + "to vertex [" + this.next()!.startingVertex.id + "]. ";

        msg += "Parent face [" + this.parentLoop.parentFace.id + "]. ";
        if (this.parentEdge === null) {
            msg = msg + "<without parent edge>. ";
        } else {
            msg = msg + "Parent edge " + this.parentEdge.id + " ";
            if (this === this.parentEdge.leftHalf) {
                msg = msg + "(left)";
            } else if (this === this.parentEdge.rightHalf) {
                msg = msg + "(right)";
            } else {
                msg = msg + "(INCONSISTENT!)";
            }
            msg = msg + ". ";
        }
        msg = msg + "Next halfedge: " + this.next()!.id + ".";
        return msg;
    }
}
