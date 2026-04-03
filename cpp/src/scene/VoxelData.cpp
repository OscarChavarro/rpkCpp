#include "scene/VoxelData.h"

VoxelData::VoxelData(Patch *data, unsigned flags) {
    this->geometry = nullptr;
    this->patch = data;
    this->voxelGrid = nullptr;
    this->flags = flags;
}

VoxelData::VoxelData(Geometry *data, unsigned flags) {
    this->geometry = data;
    this->patch = nullptr;
    this->voxelGrid = nullptr;
    this->flags = flags;
}

VoxelData::VoxelData(VoxelGrid *data, unsigned flags) {
    this->geometry = nullptr;
    this->patch = nullptr;
    this->voxelGrid = data;
    this->flags = flags;
}

VoxelData::~VoxelData() {
}
