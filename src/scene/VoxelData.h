#ifndef __VOXEL_DATA__
#define __VOXEL_DATA__

#define VOXEL_DATA_PATCH_MASK 0x10000000
#define VOXEL_DATA_GEOMETRY_MASK 0x20000000
#define VOXEL_DATA_GRID_MASK 0x40000000
#define VOXEL_DATA_RAY_COUNT_MASK 0x0fffffff

class VoxelGrid;
class Geometry;
class Patch;

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

    inline void
    updateRayId(const unsigned int id) {
        flags = (flags & ~VOXEL_DATA_RAY_COUNT_MASK) | (id & VOXEL_DATA_RAY_COUNT_MASK);
    }

    inline unsigned int
    lastRayId() const {
        return flags & VOXEL_DATA_RAY_COUNT_MASK;
    }

    inline bool
    isPatch() const {
        return flags & VOXEL_DATA_PATCH_MASK;
    }

    inline bool
    isGeom() const {
        return flags & VOXEL_DATA_GEOMETRY_MASK;
    }

    inline bool
    isGrid() const {
        return flags & VOXEL_DATA_GRID_MASK;
    }

  public:
    friend VoxelGrid;
};

#include "scene/VoxelGrid.h"
#include "skin/Geometry.h"
#include "skin/Patch.h"

#endif
