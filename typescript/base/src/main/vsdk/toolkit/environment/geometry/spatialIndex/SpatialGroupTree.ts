//= References:                                                             =
//= [SNYD1987] Snyder, John M. and Barr, Alan H. "Ray Tracing Complex       =
//=     Models Containing Surface Tessellations". SIGGRAPH '87, Computer    =
//=     Graphics 21(4), 1987, pp. 119-128.                                  =
//=     PDF: doc/references/[SNYD1987]_*.pdf                                =

import { ArrayList } from "../../../../../java/util/ArrayList.js";

/**
Groups over the primitives of a spatial index, for the indexes that keep the
caller's grouping (`GridSpatialIndex`): a tree whose root is group 0, in
which every group has a box and an ordered list of members, each one a
sub-group or a primitive. A primitive is a member of at most one group; a
primitive that is a member of no group is not indexed. The groups play the
role of the lists of [SNYD1987] section 3.2: a model built bottom up from
lists and grids, each with its bounding box.

Built by the caller (`addGroup`, `addPrimitive`) before an index is built
from it, and only read afterwards. An index copies what it needs, so the
tree may be discarded once the index exists.

Boxes are six values (lo x, y, z, hi x, y, z) as the caller computes them;
the index that reads them applies its own padding. A group without a box is
unbounded: nothing prunes it.
*/
export class SpatialGroupTree {
    private readonly primitiveBoxes: ArrayList<number>;
    private readonly groupBoxes: ArrayList<number>;
    private readonly groupBounded: ArrayList<number>;
    private readonly groupParent: ArrayList<number>;
    private readonly groupMembers: ArrayList<ArrayList<number>>;
    private readonly groupPrimitiveCount: ArrayList<number>;
    private readonly primitiveGrouped: ArrayList<number>;
    private readonly numberOfPrimitives: number;

    /**
    @param inPrimitiveBoxes six values per primitive, ids 0 .. n - 1
    @param rootBox six values, or null for an unbounded root group
    */
    public constructor(inPrimitiveBoxes: ArrayList<number>, rootBox: ArrayLike<number> | null) {
        this.primitiveBoxes = new ArrayList<number>(inPrimitiveBoxes);
        this.groupBoxes = new ArrayList<number>();
        this.groupBounded = new ArrayList<number>();
        this.groupParent = new ArrayList<number>();
        this.groupMembers = new ArrayList<ArrayList<number>>();
        this.groupPrimitiveCount = new ArrayList<number>();
        this.primitiveGrouped = new ArrayList<number>();
        this.numberOfPrimitives = Math.trunc(inPrimitiveBoxes.size() / 6);
        for (let i = 0; i < this.numberOfPrimitives; i++) {
            this.primitiveGrouped.add(0);
        }
        this.newGroup(-1, rootBox);
    }

    private newGroup(parent: number, box: ArrayLike<number> | null): number {
        const group = this.groupParent.size();
        for (let k = 0; k < 6; k++) {
            this.groupBoxes.add(box !== null ? box[k]! : 0.0);
        }
        this.groupBounded.add(box !== null ? 1 : 0);
        this.groupParent.add(parent);
        this.groupMembers.add(new ArrayList<number>());
        this.groupPrimitiveCount.add(0);
        return group;
    }

    /**
    Adds a group as the last member of `parent`.
    @param box six values, or null for an unbounded group
    @return the id of the new group, or -1 when `parent` is not a group
    */
    public addGroup(parent: number, box: ArrayLike<number> | null): number {
        if (parent < 0 || parent >= this.getNumberOfGroups()) {
            return -1;
        }
        const group = this.newGroup(parent, box);
        this.groupMembers.get(parent).add(-group - 1);
        return group;
    }

    /**
    Adds a primitive as the last member of `group`.
    @return false, and nothing is added, when the group or the primitive does
    not exist or the primitive is already a member of a group
    */
    public addPrimitive(group: number, primitive: number): boolean {
        if (
            group < 0 ||
            group >= this.getNumberOfGroups() ||
            primitive < 0 ||
            primitive >= this.numberOfPrimitives ||
            this.primitiveGrouped.get(primitive) !== 0
        ) {
            return false;
        }
        this.primitiveGrouped.set(primitive, 1);
        this.groupMembers.get(group).add(primitive);
        for (let g = group; g >= 0; g = this.groupParent.get(g)) {
            this.groupPrimitiveCount.set(g, this.groupPrimitiveCount.get(g) + 1);
        }
        return true;
    }

    public getNumberOfPrimitives(): number {
        return this.numberOfPrimitives;
    }

    public getNumberOfGroups(): number {
        return this.groupParent.size();
    }

    public getPrimitiveBoxes(): ArrayList<number> {
        return this.primitiveBoxes;
    }

    public isBounded(group: number): boolean {
        return this.groupBounded.get(group) !== 0;
    }

    /** @param coordinate 0 .. 5: lo x, y, z, hi x, y, z */
    public getGroupBox(group: number, coordinate: number): number {
        return this.groupBoxes.get(6 * group + coordinate);
    }

    /** @return the parent group, -1 for the root */
    public getParent(group: number): number {
        return this.groupParent.get(group);
    }

    public getNumberOfMembers(group: number): number {
        return this.groupMembers.get(group).size();
    }

    /**
    @return member `i` of the group: a primitive id (not negative), or a
    group, encoded as -(group + 1) (see `isGroupMember` and `memberGroup`)
    */
    public getMember(group: number, i: number): number {
        return this.groupMembers.get(group).get(i);
    }

    /** Number of primitives below the group, at any depth */
    public getPrimitiveCount(group: number): number {
        return this.groupPrimitiveCount.get(group);
    }

    public static isGroupMember(member: number): boolean {
        return member < 0;
    }

    public static memberGroup(member: number): number {
        return -member - 1;
    }
}
