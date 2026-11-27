#include "vsdk/toolkit/environment/geometry/elements/FaceColorTable.h"

FaceColorTable::FaceColorTable(const int inNumberOfFaces):
    colors(nullptr),
    numberOfFaces(inNumberOfFaces)
{
    colors = new ColorRgb[numberOfFaces > 0 ? numberOfFaces : 1];
    for ( int i = 0; i < numberOfFaces; i++ ) {
        colors[i] = ColorRgb(0.0, 0.0, 0.0);
    }
}

FaceColorTable::~FaceColorTable() {
    delete[] colors;
}
