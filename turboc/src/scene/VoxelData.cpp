#include "scene/VoxelData.h"

VoxelData::VoxelData(Patch *data, unsigned flags) {
    this->geometry = NULL;
    this->patch = data;
    this->voxelGrid = NULL;
    this->flags = flags;
}

VoxelData::VoxelData(Geometry *data, unsigned flags) {
    this->geometry = data;
    this->patch = NULL;
    this->voxelGrid = NULL;
    this->flags = flags;
}

VoxelData::VoxelData(VoxelGrid *data, unsigned flags) {
    this->geometry = NULL;
    this->patch = NULL;
    this->voxelGrid = data;
    this->flags = flags;
}

VoxelData::~VoxelData() {
}
