#include "vsdk/toolkit/scene/VoxelData.h"

VoxelData::VoxelData(Patch *data, const unsigned flags, const int inIndex):
    patch(data),
    geometry(NULL),
    voxelGrid(NULL),
    flags(flags),
    index(inIndex)
{
}

VoxelData::VoxelData(Geometry *data, const unsigned flags, const int inIndex):
    patch(NULL),
    geometry(data),
    voxelGrid(NULL),
    flags(flags),
    index(inIndex)
{
}

VoxelData::VoxelData(VoxelGrid *data, const unsigned flags, const int inIndex):
    patch(NULL),
    geometry(NULL),
    voxelGrid(data),
    flags(flags),
    index(inIndex)
{
}

VoxelData::~VoxelData() {
}
