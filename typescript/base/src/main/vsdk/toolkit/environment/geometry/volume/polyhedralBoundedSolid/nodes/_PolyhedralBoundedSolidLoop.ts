//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

import { CircularArrayList } from "../../../../../common/dataStructures/CircularArrayList.js";
import { FundamentalEntity } from "../../../../../common/FundamentalEntity.js";
import type { ArrayLinkable } from "../../../../../common/dataStructures/ArrayLinkable.js";
import type { _PolyhedralBoundedSolidEdge } from "./_PolyhedralBoundedSolidEdge.js";
import type { _PolyhedralBoundedSolidFace } from "./_PolyhedralBoundedSolidFace.js";
import type { _PolyhedralBoundedSolidHalfEdge } from "./_PolyhedralBoundedSolidHalfEdge.js";

/**
As noted in [MANT1988].10.2.1, a `_PolyhedralBoundedSolidLoop` describes
one connected boundary inside a `_PolyhedralBoundedSolidFace`.

Note that in the sake of simplify and eficiency current programming
implementation of this class exhibit public access attributes. It is important
to note that those attributes will only be accessed directly from related
classes in the same package
(vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes) and
from methods in the `PolyhedralBoundedSolid` class, and that they should
not be used from outer classes.
*/
export class _PolyhedralBoundedSolidLoop extends FundamentalEntity implements ArrayLinkable {
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
    public parentFace!: _PolyhedralBoundedSolidFace;

    /// As noted in [MANT1988].10.2.3, consider that there is a special
    /// case for empty loops. Note that this case doesn't affect this
    /// reference.
    public boundaryStartHalfEdge: _PolyhedralBoundedSolidHalfEdge | null = null;

    public halfEdgesList!: CircularArrayList<_PolyhedralBoundedSolidHalfEdge>;

    //=================================================================
    public constructor(parent: _PolyhedralBoundedSolidFace) {
        super();
        this.init(parent);
    }

    private init(parent: _PolyhedralBoundedSolidFace): void {
        this.parentFace = parent;
        this.parentFace.boundariesList.add(this);
        this.halfEdgesList = new CircularArrayList<_PolyhedralBoundedSolidHalfEdge>();
    }

    public unlistHalfEdge(he: _PolyhedralBoundedSolidHalfEdge): void {
        if (this.halfEdgesList.locateWindowAtElem(he)) {
            this.halfEdgesList.removeElemAtWindow();
        }
        if (this.halfEdgesList.size() > 0) {
            this.boundaryStartHalfEdge = this.halfEdgesList.get(0);
        } else {
            this.boundaryStartHalfEdge = null;
        }
    }

    /**
    Locates a half edge that goes from vertex with id `a` to vertex with
    id `b`.  Returns null if no such half edge exists in this loop.
    @param a
    @param b
    @return requested halfedge
    */
    public halfEdgeVertices(a: number, b: number): _PolyhedralBoundedSolidHalfEdge | null {
        let he: _PolyhedralBoundedSolidHalfEdge | null;
        let oldhe: _PolyhedralBoundedSolidHalfEdge;
        he = this.boundaryStartHalfEdge;
        do {
            oldhe = he!;
            he = he!.next();
            if (he === null) {
                // Loop is not closed!
                break;
            }

            if (oldhe.startingVertex.id === a && he.startingVertex.id === b) {
                return oldhe;
            }
        } while (he !== this.boundaryStartHalfEdge);
        return null;
    }

    /** Locates a half edge that goes from vertex with id `a` to vertex with
    id `b`.  Returns null if no such half edge exists in this loop. */
    public firstHalfEdgeAtVertex(a: number): _PolyhedralBoundedSolidHalfEdge | null {
        let he: _PolyhedralBoundedSolidHalfEdge | null;
        let oldhe: _PolyhedralBoundedSolidHalfEdge;
        he = this.boundaryStartHalfEdge;
        do {
            oldhe = he!;
            he = he!.next();
            if (he === null) {
                // Loop is not closed!
                break;
            }

            if (oldhe.startingVertex.id === a) {
                return oldhe;
            }
        } while (he !== this.boundaryStartHalfEdge);
        return null;
    }

    /**
    Vitral SDK's current implementation of original `delhe` utility function
    presented at program [MANT1988].11.4. and section [MANT1988].11.2.2.
    Note that current implementation is quite diferent from the original from
    [MANT1988]. This could lead to subtle problems! This method's functionality
    should be better understood!
    */
    public delhe(he: _PolyhedralBoundedSolidHalfEdge): void {
        this.halfEdgesList.locateWindowAtElem(he);
        this.halfEdgesList.removeElemAtWindow();

        if (this.halfEdgesList.size() > 0) {
            this.boundaryStartHalfEdge = this.halfEdgesList.get(0);
        }
    }

    public revert(): void {
        if (this.halfEdgesList.size() <= 0) {
            return;
        }

        //-----------------------------------------------------------------
        let he: _PolyhedralBoundedSolidHalfEdge;
        const edges: (_PolyhedralBoundedSolidEdge | null)[] = new Array(this.halfEdgesList.size()).fill(null);
        const sides: boolean[] = new Array(this.halfEdgesList.size()).fill(false);
        let i: number;

        for (i = 0; i < this.halfEdgesList.size(); i++) {
            he = this.halfEdgesList.get(i)!;
            // Degenerate loops can be left behind transiently after lkemr
            // during boolean finishing. They are not traversable via an edge.
            if (he.parentEdge === null) {
                return;
            }
            edges[i] = he.parentEdge;
            sides[i] = false;
            if (he.parentEdge.rightHalf === he) {
                sides[i] = true;
            }
        }

        for (i = 1; i < this.halfEdgesList.size(); i++) {
            this.halfEdgesList.get(i)!.parentEdge = edges[i - 1]!;
            if (sides[i - 1]) {
                edges[i - 1]!.rightHalf = this.halfEdgesList.get(i);
            } else {
                edges[i - 1]!.leftHalf = this.halfEdgesList.get(i);
            }
        }
        this.halfEdgesList.get(0)!.parentEdge = edges[i - 1]!;
        if (sides[i - 1]) {
            edges[i - 1]!.rightHalf = this.halfEdgesList.get(0);
        } else {
            edges[i - 1]!.leftHalf = this.halfEdgesList.get(0);
        }

        //-----------------------------------------------------------------
        this.halfEdgesList.reverse();
    }

    public override toString(): string {
        let msg: string;

        msg = "Loop, parent face " + this.parentFace.id;

        return msg;
    }
}
