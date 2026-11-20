#include "vsdk/toolkit/scene/VoxelData.h"

VoxelData::VoxelData(Patch *data, const unsigned flags, const int inIndex):
    patch(data),
    geometry(nullptr),
    voxelGrid(nullptr),
    flags(flags),
    index(inIndex)
{
}

VoxelData::VoxelData(RpkGeometry *data, const unsigned flags, const int inIndex):
    patch(nullptr),
    geometry(data),
    voxelGrid(nullptr),
    flags(flags),
    index(inIndex)
{
}

VoxelData::VoxelData(VoxelGrid *data, const unsigned flags, const int inIndex):
    patch(nullptr),
    geometry(nullptr),
    voxelGrid(data),
    flags(flags),
    index(inIndex)
{
}

VoxelData::~VoxelData() {
}
