//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

import { FundamentalEntity } from "../../../../../common/FundamentalEntity.js";
import type { ArrayLinkable } from "../../../../../common/dataStructures/ArrayLinkable.js";
import { ColorRgb } from "../../../../../common/color/ColorRgb.js";
import type { PolyhedralBoundedSolid } from "../PolyhedralBoundedSolid.js";
import type { _PolyhedralBoundedSolidHalfEdge } from "./_PolyhedralBoundedSolidHalfEdge.js";

/**
As noted in [MANT1988].10.2.2, a `_PolyhedralBoundedSolidEdge` makes a
face-to-face relationship representing the identification of the line
segments between faces.

Note that in the sake of simplify and efficiency current programming
implementation this class exhibit public access attributes. It is important
to note that those attributes will only be accessed directly from related
classes in the same package
(vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes) and
from methods in the `PolyhedralBoundedSolid` class, and that they should
not be used from outer classes.
*/
export class _PolyhedralBoundedSolidEdge extends FundamentalEntity implements ArrayLinkable {
    /// Place of this element in the list that holds it, kept up to date by
    /// the list (see `ArrayLinkable`).
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

    /// Reference to `right` half edge, as defined in [MANT1988].10.2.2.
    /// Note that half edge in this side is considered positively oriented.
    public rightHalf!: _PolyhedralBoundedSolidHalfEdge | null;

    /// Reference to `right` half edge, as defined in [MANT1988].10.2.2.
    /// Note that half edge in this side is considered negatively oriented.
    public leftHalf!: _PolyhedralBoundedSolidHalfEdge | null;

    //
    public id!: number;
    public debugColor!: ColorRgb;
    private static currentId = 1;

    //=================================================================
    public constructor(parentSolid: PolyhedralBoundedSolid) {
        super();
        this.init(parentSolid);
    }

    private init(parentSolid: PolyhedralBoundedSolid): void {
        parentSolid.getEdgesList().add(this);
        this.rightHalf = null;
        this.leftHalf = null;

        this.id = _PolyhedralBoundedSolidEdge.currentId;
        _PolyhedralBoundedSolidEdge.currentId++;

        this.debugColor = new ColorRgb(1, 1, 1);
    }

    public getEndingVertexId(): number {
        if (this.leftHalf === null) {
            return -1;
        }
        return this.leftHalf.startingVertex.id;
    }

    public getStartingVertexId(): number {
        if (this.rightHalf === null) {
            return -1;
        }
        return this.rightHalf.startingVertex.id;
    }

    public override toString(): string {
        let msg: string;
        msg = "Edge id " + this.id + ". Half1: ";
        if (this.leftHalf === null) {
            msg = msg + "null. ";
        } else {
            msg = msg + "vertex " + this.leftHalf.startingVertex.id;
        }

        msg = msg + " / Half2: ";

        if (this.rightHalf === null) {
            msg = msg + "null. ";
        } else {
            msg = msg + "vertex " + this.rightHalf.startingVertex.id;
        }

        return msg;
    }
}
