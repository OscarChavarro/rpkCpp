#include "vsdk/toolkit/scene/FaceVisibilityTable.h"

FaceVisibilityTable::FaceVisibilityTable(const int inNumberOfFaces):
    visible(new bool[inNumberOfFaces > 0 ? inNumberOfFaces : 1]),
    numberOfFaces(inNumberOfFaces)
{
    for ( int i = 0; i < numberOfFaces; i++ ) {
        visible[i] = false;
    }
}

FaceVisibilityTable::~FaceVisibilityTable() {
    delete[] visible;
}
