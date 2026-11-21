#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/environment/geometry/spatialIndex/SpatialGroupTree.h"

SpatialGroupTree::SpatialGroupTree(const java::ArrayList<double> &inPrimitiveBoxes, const double *rootBox) :
    primitiveBoxes(inPrimitiveBoxes),
    groupBoxes(),
    groupBounded(),
    groupParent(),
    groupMembers(),
    groupPrimitiveCount(),
    primitiveGrouped(),
    numberOfPrimitives(static_cast<int>(inPrimitiveBoxes.size() / 6))
{
    for ( int i = 0; i < numberOfPrimitives; i++ ) {
        primitiveGrouped.add(0);
    }
    newGroup(-1, rootBox);
}

SpatialGroupTree::~SpatialGroupTree()
{
    for ( long int i = 0; i < groupMembers.size(); i++ ) {
        delete groupMembers[i];
    }
}

int
SpatialGroupTree::newGroup(const int parent, const double *box)
{
    const int group = static_cast<int>(groupParent.size());
    for ( int k = 0; k < 6; k++ ) {
        groupBoxes.add(box != nullptr ? box[k] : 0.0);
    }
    groupBounded.add(box != nullptr ? 1 : 0);
    groupParent.add(parent);
    groupMembers.add(new java::ArrayList<int>());
    groupPrimitiveCount.add(0);
    return group;
}

int
SpatialGroupTree::addGroup(const int parent, const double *box)
{
    if ( parent < 0 || parent >= getNumberOfGroups() ) {
        return -1;
    }
    const int group = newGroup(parent, box);
    groupMembers[parent]->add(-group - 1);
    return group;
}

bool
SpatialGroupTree::addPrimitive(const int group, const int primitive)
{
    if ( group < 0 || group >= getNumberOfGroups()
      || primitive < 0 || primitive >= numberOfPrimitives
      || primitiveGrouped[primitive] != 0 ) {
        return false;
    }
    primitiveGrouped[primitive] = 1;
    groupMembers[group]->add(primitive);
    for ( int g = group; g >= 0; g = groupParent[g] ) {
        groupPrimitiveCount[g]++;
    }
    return true;
}

int
SpatialGroupTree::getNumberOfPrimitives() const
{
    return numberOfPrimitives;
}

int
SpatialGroupTree::getNumberOfGroups() const
{
    return static_cast<int>(groupParent.size());
}

const java::ArrayList<double> &
SpatialGroupTree::getPrimitiveBoxes() const
{
    return primitiveBoxes;
}

bool
SpatialGroupTree::isBounded(const int group) const
{
    return groupBounded[group] != 0;
}

double
SpatialGroupTree::getGroupBox(const int group, const int coordinate) const
{
    return groupBoxes[6 * group + coordinate];
}

int
SpatialGroupTree::getParent(const int group) const
{
    return groupParent[group];
}

int
SpatialGroupTree::getNumberOfMembers(const int group) const
{
    return static_cast<int>(groupMembers[group]->size());
}

int
SpatialGroupTree::getMember(const int group, const int i) const
{
    return (*groupMembers[group])[i];
}

int
SpatialGroupTree::getPrimitiveCount(const int group) const
{
    return groupPrimitiveCount[group];
}
