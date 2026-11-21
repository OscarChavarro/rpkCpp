import type { ArrayList } from "../../../../../java/util/ArrayList";

/**
Groups over the primitives of a spatial index, for the indexes that keep the
caller's grouping (`GridSpatialIndex`): a tree whose root is group 0, in
which every group has a box and an ordered list of members, each one a
sub-group or a primitive. A primitive is a member of at most one group; a
primitive that is a member of no group is not indexed.

Built by the caller (`addGroup`, `addPrimitive`) before an index is built
from it, and only read afterwards. An index copies what it needs, so the
tree may be discarded once the index exists.

Boxes are six values (lo x, y, z, hi x, y, z) as the caller computes them;
the index that reads them applies its own padding. A group without a box is
unbounded: nothing prunes it.
*/
export class SpatialGroupTree {
  private readonly primitiveBoxes: number[];
  private readonly groupBoxes: number[];
  private readonly groupBounded: boolean[];
  private readonly groupParent: number[];
  private readonly groupMembers: number[][];
  private readonly groupPrimitiveCount: number[];
  private readonly primitiveGrouped: boolean[];
  private readonly numberOfPrimitives: number;

  /**
  @param inPrimitiveBoxes six values per primitive, ids 0 .. n - 1
  @param rootBox six values, or null for an unbounded root group
  */
  public constructor(inPrimitiveBoxes: ArrayList<number>, rootBox: number[] | null) {
    this.primitiveBoxes = inPrimitiveBoxes.toArray();
    this.groupBoxes = [];
    this.groupBounded = [];
    this.groupParent = [];
    this.groupMembers = [];
    this.groupPrimitiveCount = [];
    this.primitiveGrouped = [];
    this.numberOfPrimitives = globalThis.Math.trunc(inPrimitiveBoxes.size() / 6);
    for (let i = 0; i < this.numberOfPrimitives; i++) {
      this.primitiveGrouped.push(false);
    }
    this.newGroup(-1, rootBox);
  }

  private newGroup(parent: number, box: number[] | null): number {
    const group = this.groupParent.length;
    for (let k = 0; k < 6; k++) {
      this.groupBoxes.push(box !== null ? box[k]! : 0.0);
    }
    this.groupBounded.push(box !== null);
    this.groupParent.push(parent);
    this.groupMembers.push([]);
    this.groupPrimitiveCount.push(0);
    return group;
  }

  /**
  Adds a group as the last member of `parent`.
  @param box six values, or null for an unbounded group
  @return the id of the new group, or -1 when `parent` is not a group
  */
  public addGroup(parent: number, box: number[] | null): number {
    if (parent < 0 || parent >= this.getNumberOfGroups()) {
      return -1;
    }
    const group = this.newGroup(parent, box);
    this.groupMembers[parent]!.push(-group - 1);
    return group;
  }

  /**
  Adds a primitive as the last member of `group`.
  @return false, and nothing is added, when the group or the primitive does
  not exist or the primitive is already a member of a group
  */
  public addPrimitive(group: number, primitive: number): boolean {
    if (group < 0 || group >= this.getNumberOfGroups()
      || primitive < 0 || primitive >= this.numberOfPrimitives
      || this.primitiveGrouped[primitive]) {
      return false;
    }
    this.primitiveGrouped[primitive] = true;
    this.groupMembers[group]!.push(primitive);
    for (let g = group; g >= 0; g = this.groupParent[g]!) {
      this.groupPrimitiveCount[g]!++;
    }
    return true;
  }

  public getNumberOfPrimitives(): number {
    return this.numberOfPrimitives;
  }

  public getNumberOfGroups(): number {
    return this.groupParent.length;
  }

  public getPrimitiveBoxes(): number[] {
    return this.primitiveBoxes;
  }

  public isBounded(group: number): boolean {
    return this.groupBounded[group]!;
  }

  /** @param coordinate 0 .. 5: lo x, y, z, hi x, y, z */
  public getGroupBox(group: number, coordinate: number): number {
    return this.groupBoxes[6 * group + coordinate]!;
  }

  /** @return the parent group, -1 for the root */
  public getParent(group: number): number {
    return this.groupParent[group]!;
  }

  public getNumberOfMembers(group: number): number {
    return this.groupMembers[group]!.length;
  }

  /**
  @return member `i` of the group: a primitive id (not negative), or a
  group, encoded as -(group + 1) (see `isGroupMember` and `memberGroup`)
  */
  public getMember(group: number, i: number): number {
    return this.groupMembers[group]![i]!;
  }

  /** Number of primitives below the group, at any depth */
  public getPrimitiveCount(group: number): number {
    return this.groupPrimitiveCount[group]!;
  }

  public static isGroupMember(member: number): boolean {
    return member < 0;
  }

  public static memberGroup(member: number): number {
    return -member - 1;
  }
}
