#include "vsdk/toolkit/scene/FaceBoundsTable.h"

FaceBoundsTable::FaceBoundsTable(Patch *const *faces, const int inNumberOfFaces):
    bounds(NULL),
    numberOfFaces(inNumberOfFaces)
{
    bounds = new BoundingBox[numberOfFaces > 0 ? numberOfFaces : 1];
    for ( int i = 0; i < numberOfFaces; i++ ) {
        faces[i]->computeAndGetBoundingBox(&bounds[i]);
    }
}

FaceBoundsTable::~FaceBoundsTable() {
    delete[] bounds;
    bounds = NULL;
}
