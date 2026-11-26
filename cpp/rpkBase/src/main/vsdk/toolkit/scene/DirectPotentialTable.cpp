#include "vsdk/toolkit/scene/DirectPotentialTable.h"

DirectPotentialTable::DirectPotentialTable(const int inNumberOfFaces):
    potentials(new double[inNumberOfFaces]),
    numberOfFaces(inNumberOfFaces)
{
    for ( int i = 0; i < numberOfFaces; i++ ) {
        potentials[i] = 0.0;
    }
}

DirectPotentialTable::~DirectPotentialTable() {
    delete[] potentials;
}
