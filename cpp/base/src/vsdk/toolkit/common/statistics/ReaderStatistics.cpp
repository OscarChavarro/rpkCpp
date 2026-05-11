#include "vsdk/toolkit/common/statistics/ReaderStatistics.h"

ReaderStatistics::ReaderStatistics():
    numberOfGeometries(0),
    numberOfCompounds(0),
    numberOfSurfaces(0),
    numberOfVertices(0),
    numberOfPatches(0),
    numberOfElements(0),
    numberOfLightSources(0)
{
}

void
ReaderStatistics::reset() {
    numberOfGeometries = 0;
    numberOfCompounds = 0;
    numberOfSurfaces = 0;
    numberOfVertices = 0;
    numberOfPatches = 0;
    numberOfElements = 0;
    numberOfLightSources = 0;
}
