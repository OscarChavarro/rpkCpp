package vsdk.toolkit.environment.geometry.spatialIndex;

import java.util.ArrayList;

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
public final class SpatialGroupTree {
    private final double[] primitiveBoxes;
    private final ArrayList<Double> groupBoxes;
    private final ArrayList<Boolean> groupBounded;
    private final ArrayList<Integer> groupParent;
    private final ArrayList<ArrayList<Integer>> groupMembers;
    private final ArrayList<Integer> groupPrimitiveCount;
    private final boolean[] primitiveGrouped;
    private final int numberOfPrimitives;

    /**
    @param inPrimitiveBoxes six values per primitive, ids 0 .. n - 1
    @param rootBox six values, or null for an unbounded root group
    */
    public SpatialGroupTree(double[] inPrimitiveBoxes, double[] rootBox) {
        primitiveBoxes = inPrimitiveBoxes.clone();
        groupBoxes = new ArrayList<>();
        groupBounded = new ArrayList<>();
        groupParent = new ArrayList<>();
        groupMembers = new ArrayList<>();
        groupPrimitiveCount = new ArrayList<>();
        numberOfPrimitives = inPrimitiveBoxes.length / 6;
        primitiveGrouped = new boolean[numberOfPrimitives];
        newGroup(-1, rootBox);
    }

    private int newGroup(int parent, double[] box) {
        final int group = groupParent.size();
        for ( int k = 0; k < 6; k++ ) {
            groupBoxes.add(box != null ? box[k] : 0.0);
        }
        groupBounded.add(box != null);
        groupParent.add(parent);
        groupMembers.add(new ArrayList<>());
        groupPrimitiveCount.add(0);
        return group;
    }

    /**
    Adds a group as the last member of `parent`.
    @param box six values, or null for an unbounded group
    @return the id of the new group, or -1 when `parent` is not a group
    */
    public int addGroup(int parent, double[] box) {
        if ( parent < 0 || parent >= getNumberOfGroups() ) {
            return -1;
        }
        final int group = newGroup(parent, box);
        groupMembers.get(parent).add(-group - 1);
        return group;
    }

    /**
    Adds a primitive as the last member of `group`.
    @return false, and nothing is added, when the group or the primitive does
    not exist or the primitive is already a member of a group
    */
    public boolean addPrimitive(int group, int primitive) {
        if ( group < 0 || group >= getNumberOfGroups()
          || primitive < 0 || primitive >= numberOfPrimitives
          || primitiveGrouped[primitive] ) {
            return false;
        }
        primitiveGrouped[primitive] = true;
        groupMembers.get(group).add(primitive);
        for ( int g = group; g >= 0; g = groupParent.get(g) ) {
            groupPrimitiveCount.set(g, groupPrimitiveCount.get(g) + 1);
        }
        return true;
    }

    public int getNumberOfPrimitives() {
        return numberOfPrimitives;
    }

    public int getNumberOfGroups() {
        return groupParent.size();
    }

    /** Six values per primitive; read only */
    public double[] getPrimitiveBoxes() {
        return primitiveBoxes;
    }

    public boolean isBounded(int group) {
        return groupBounded.get(group);
    }

    /** @param coordinate 0 .. 5: lo x, y, z, hi x, y, z */
    public double getGroupBox(int group, int coordinate) {
        return groupBoxes.get(6 * group + coordinate);
    }

    /** @return the parent group, -1 for the root */
    public int getParent(int group) {
        return groupParent.get(group);
    }

    public int getNumberOfMembers(int group) {
        return groupMembers.get(group).size();
    }

    /**
    @return member `i` of the group: a primitive id (not negative), or a
    group, encoded as -(group + 1) (see `isGroupMember` and `memberGroup`)
    */
    public int getMember(int group, int i) {
        return groupMembers.get(group).get(i);
    }

    /** Number of primitives below the group, at any depth */
    public int getPrimitiveCount(int group) {
        return groupPrimitiveCount.get(group);
    }

    public static boolean isGroupMember(int member) {
        return member < 0;
    }

    public static int memberGroup(int member) {
        return -member - 1;
    }
}
