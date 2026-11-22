#include "vsdk/toolkit/scene/DirectPotentialTable.h"

DirectPotentialTable::DirectPotentialTable(const int inNumberOfFaces):
    potentials(new float[inNumberOfFaces]),
    numberOfFaces(inNumberOfFaces)
{
    for ( int i = 0; i < numberOfFaces; i++ ) {
        potentials[i] = 0.0F;
    }
}

DirectPotentialTable::~DirectPotentialTable() {
    delete[] potentials;
}
