//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

import { FundamentalEntity } from "../../../../../common/FundamentalEntity.js";
import type { ArrayLinkable } from "../../../../../common/dataStructures/ArrayLinkable.js";
import { Vector3Dd } from "../../../../../common/linealAlgebra/Vector3Dd.js";
import { ColorRgb } from "../../../../../common/color/ColorRgb.js";
import type { PolyhedralBoundedSolid } from "../PolyhedralBoundedSolid.js";
import type { _PolyhedralBoundedSolidHalfEdge } from "./_PolyhedralBoundedSolidHalfEdge.js";

/**
As noted in [MANT1988].10.2.2, a `_PolyhedralBoundedSolidVertex` contains
a vertex position for the geometric information of the boundary model,
and a reference to one of the half-edges emanating from it.
*/
export class _PolyhedralBoundedSolidVertex extends FundamentalEntity implements ArrayLinkable {
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

    /// Defined as presented in [MANT1988].10.2.1
    public id!: number;

    /// Defined as presented in [MANT1988].10.2.1
    public position!: Vector3Dd;

    /// Defined as presented in [MANT1988].10.2.2
    public emanatingHalfEdge!: _PolyhedralBoundedSolidHalfEdge | null;

    public debugColor!: ColorRgb;

    //=================================================================
    public constructor(parentSolid: PolyhedralBoundedSolid, position: Vector3Dd, id: number) {
        super();
        this.init(parentSolid, position, id);
    }

    private init(parentSolid: PolyhedralBoundedSolid, position: Vector3Dd, id: number): void {
        this.id = id;
        this.emanatingHalfEdge = null;
        this.position = new Vector3Dd(position);
        parentSolid.getVerticesList().add(this);
        this.debugColor = new ColorRgb(1, 0, 0);
    }

    public override toString(): string {
        let msg: string;
        msg = "vertex id " + this.id + ". Position " + this.position + ". ";
        if (this.emanatingHalfEdge === null) {
            msg = msg + "No associated half-edge.";
        } else {
            msg = msg + "H.E. " + this.emanatingHalfEdge.id;
        }
        return msg;
    }
}
