#include "vsdk/toolkit/scene/VoxelGridWorkspace.h"

VoxelGridWorkspace::VoxelGridWorkspace(const int inNumberOfItems, VisibilityQueryRecorder *inRecorder):
    lastRayIdByItem(nullptr),
    numberOfItems(inNumberOfItems),
    rayCount(0),
    recorder(inRecorder)
{
    lastRayIdByItem = new unsigned int[numberOfItems > 0 ? numberOfItems : 1];
    for ( int i = 0; i < numberOfItems; i++ ) {
        lastRayIdByItem[i] = 0;
    }
}

VoxelGridWorkspace::~VoxelGridWorkspace() {
    delete[] lastRayIdByItem;
    lastRayIdByItem = nullptr;
}
