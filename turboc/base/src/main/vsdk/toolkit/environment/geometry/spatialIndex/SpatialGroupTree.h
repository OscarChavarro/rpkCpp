#ifndef __SPATIAL_GROUP_TREE__
#define __SPATIAL_GROUP_TREE__

#include "vsdk/toolkit/common/VSDK.h"
#include "java/util/ArrayList.h"

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
class SpatialGroupTree {
  private:
    ArrayList<double> primitiveBoxes;
    ArrayList<double> groupBoxes;
    ArrayList<char> groupBounded;
    ArrayList<int> groupParent;
    ArrayList<ArrayList<int> *> groupMembers;
    ArrayList<int> groupPrimitiveCount;
    ArrayList<char> primitiveGrouped;
    int numberOfPrimitives;

    int newGroup(int parent, const double *box);

    // Not copyable
    SpatialGroupTree(const SpatialGroupTree &);
    SpatialGroupTree &operator=(const SpatialGroupTree &);

  public:
    /**
    @param inPrimitiveBoxes six values per primitive, ids 0 .. n - 1
    @param rootBox six values, or null for an unbounded root group
    */
    SpatialGroupTree(const ArrayList<double> &inPrimitiveBoxes, const double *rootBox);
    ~SpatialGroupTree();

    /**
    Adds a group as the last member of `parent`.
    @param box six values, or null for an unbounded group
    @return the id of the new group, or -1 when `parent` is not a group
    */
    int addGroup(int parent, const double *box);

    /**
    Adds a primitive as the last member of `group`.
    @return FALSE, and nothing is added, when the group or the primitive does
    not exist or the primitive is already a member of a group
    */
    BOOLEAN addPrimitive(int group, int primitive);

    int getNumberOfPrimitives() const;
    int getNumberOfGroups() const;
    const ArrayList<double> &getPrimitiveBoxes() const;
    BOOLEAN isBounded(int group) const;

    /** @param coordinate 0 .. 5: lo x, y, z, hi x, y, z */
    double getGroupBox(int group, int coordinate) const;

    /** @return the parent group, -1 for the root */
    int getParent(int group) const;
    int getNumberOfMembers(int group) const;

    /**
    @return member `i` of the group: a primitive id (not negative), or a
    group, encoded as -(group + 1) (see `isGroupMember` and `memberGroup`)
    */
    int getMember(int group, int i) const;

    /** Number of primitives below the group, at any depth */
    int getPrimitiveCount(int group) const;

    static BOOLEAN isGroupMember(int member);
    static int memberGroup(int member);
};

inline BOOLEAN
SpatialGroupTree::isGroupMember(const int member)
{
    return member < 0;
}

inline int
SpatialGroupTree::memberGroup(const int member)
{
    return -member - 1;
}

#endif
