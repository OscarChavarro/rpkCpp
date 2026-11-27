#ifndef __VOXEL_DATA__
#define __VOXEL_DATA__

#include "vsdk/toolkit/scene/VoxelDataFlags.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

class VoxelGrid;

/**
One item stored in the cells of a VoxelGrid: a patch, a geometry or a sub
grid. The index is dense over the whole grid hierarchy and addresses the
per-item scratch of a VoxelGridWorkspace.

Shared, read-only after the grid is built.
*/
class VoxelData {
  private:
    Patch *patch;
    RpkGeometry *geometry;
    VoxelGrid *voxelGrid;
    unsigned flags; // Patch, geometry or grid
    int index;

  public:
    VoxelData(VoxelGrid *data, unsigned flags, int inIndex);
    VoxelData(Patch *data, unsigned flags, int inIndex);
    VoxelData(RpkGeometry *data, unsigned flags, int inIndex);
    virtual ~VoxelData();

    inline Patch *getPatch() const;
    inline RpkGeometry *getGeometry() const;
    inline VoxelGrid *getVoxelGrid() const;
    inline int getIndex() const;
    inline bool isPatch() const;
    inline bool isGeom() const;
    inline bool isGrid() const;
};

inline Patch *
VoxelData::getPatch() const {
    return patch;
}

inline RpkGeometry *
VoxelData::getGeometry() const {
    return geometry;
}

inline VoxelGrid *
VoxelData::getVoxelGrid() const {
    return voxelGrid;
}

inline int
VoxelData::getIndex() const {
    return index;
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
