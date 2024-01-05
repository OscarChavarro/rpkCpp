#ifndef __VOXEL_DATA__
#define __VOXEL_DATA__

class VoxelData {
public:
    void *ptr; // Patch or Geometry pointer
    unsigned flags; // patch or geometry? last ray id, ...
};

#endif
