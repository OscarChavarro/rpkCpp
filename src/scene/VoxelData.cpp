#include "scene/VoxelData.h"

VoxelData::VoxelData(void *ptr, unsigned flags) {
    this->ptr = ptr;
    this->flags = flags;
}
