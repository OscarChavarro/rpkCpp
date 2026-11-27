#ifndef __VOXEL_GRID_WORKSPACE__
#define __VOXEL_GRID_WORKSPACE__

#include "vsdk/toolkit/scene/VoxelDataFlags.h"

/**
Per-query scratch of a VoxelGrid traversal: the ray counter and, for every
item of the grid hierarchy, the counter of the last ray that tested it. Items
spanning several cells are so tested only once per ray.

Workspace: mutable, owned by one caller and passed explicitly on every
gridIntersect call. The grid itself is never written during a query.
*/
class VoxelGridWorkspace {
  private:
    unsigned int *lastRayIdByItem;
    int numberOfItems;
    unsigned int rayCount;

    // Not copyable
    VoxelGridWorkspace(const VoxelGridWorkspace &);
    VoxelGridWorkspace &operator=(const VoxelGridWorkspace &);

  public:
    explicit VoxelGridWorkspace(int inNumberOfItems);
    ~VoxelGridWorkspace();

    inline unsigned int nextRayId();
    inline unsigned int lastRayId(int itemIndex) const;
    inline void updateRayId(int itemIndex, unsigned int id);
    inline int getNumberOfItems() const;
};

inline unsigned int
VoxelGridWorkspace::nextRayId() {
    rayCount++;
    return rayCount & VOXEL_DATA_RAY_COUNT_MASK;
}

inline unsigned int
VoxelGridWorkspace::lastRayId(const int itemIndex) const {
    return lastRayIdByItem[itemIndex];
}

inline void
VoxelGridWorkspace::updateRayId(const int itemIndex, const unsigned int id) {
    lastRayIdByItem[itemIndex] = id & VOXEL_DATA_RAY_COUNT_MASK;
}

inline int
VoxelGridWorkspace::getNumberOfItems() const {
    return numberOfItems;
}

#endif
