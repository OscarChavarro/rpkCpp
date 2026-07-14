#ifndef VOXEL_DATA__
#define VOXEL_DATA__

#include "vsdk/toolkit/scene/VoxelDataFlags.h"
#include "vsdk/toolkit/skin/Geometry.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

class VoxelGrid;

class VoxelData {
  private:
    Patch *patch;
    Geometry *geometry;
    VoxelGrid *voxelGrid;

    unsigned flags; // Patch or geometry? last ray id, ...

    VoxelData(VoxelGrid *data, unsigned flags);
    VoxelData(Patch *data, unsigned flags);
    VoxelData(Geometry *data, unsigned flags);
    virtual ~VoxelData();
    void updateRayId(const unsigned int id);
    unsigned int lastRayId() const;
    bool isPatch() const;
    bool isGeom() const;
    bool isGrid() const;

  public:
    friend VoxelGrid;
};

inline void
VoxelData::updateRayId(const unsigned int id) {
    flags = (flags & ~VOXEL_DATA_RAY_COUNT_MASK) | (id & VOXEL_DATA_RAY_COUNT_MASK);
}

inline unsigned int
VoxelData::lastRayId() const {
    return flags & VOXEL_DATA_RAY_COUNT_MASK;
}

inline bool
VoxelData::isPatch() const {
    return flags & VOXEL_DATA_PATCH_MASK;
}

inline bool
VoxelData::isGeom() const {
    return flags & VOXEL_DATA_GEOMETRY_MASK;
}

inline bool
VoxelData::isGrid() const {
    return flags & VOXEL_DATA_GRID_MASK;
}

#endif
