#include "vsdk/toolkit/scene/FaceColorTable.h"

FaceColorTable::FaceColorTable(const int inNumberOfFaces):
    colors(nullptr),
    numberOfFaces(inNumberOfFaces)
{
    colors = new ColorRgbMutable[numberOfFaces > 0 ? numberOfFaces : 1];
    for ( int i = 0; i < numberOfFaces; i++ ) {
        colors[i] = ColorRgbMutable(0.0, 0.0, 0.0);
    }
}

FaceColorTable::~FaceColorTable() {
    delete[] colors;
}
