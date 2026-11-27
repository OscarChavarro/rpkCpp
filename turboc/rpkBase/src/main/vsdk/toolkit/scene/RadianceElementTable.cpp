#include "vsdk/toolkit/scene/RadianceElementTable.h"
#include <stddef.h>

RadianceElementTable::RadianceElementTable(const int inNumberOfFaces):
    elements(new Element *[inNumberOfFaces > 0 ? inNumberOfFaces : 1]),
    numberOfFaces(inNumberOfFaces)
{
    for ( int i = 0; i < numberOfFaces; i++ ) {
        elements[i] = NULL;
    }
}

RadianceElementTable::~RadianceElementTable() {
    delete[] elements;
}
