#include "vsdk/toolkit/scene/FaceBoundsTable.h"

FaceBoundsTable::FaceBoundsTable(Patch *const *faces, const int inNumberOfFaces):
    bounds(nullptr),
    numberOfFaces(inNumberOfFaces)
{
    bounds = new RpkAxisAlignedBoundingBox[numberOfFaces > 0 ? numberOfFaces : 1];
    for ( int i = 0; i < numberOfFaces; i++ ) {
        faces[i]->computeBounds(&bounds[i]);
    }
}

FaceBoundsTable::~FaceBoundsTable() {
    delete[] bounds;
    bounds = nullptr;
}
