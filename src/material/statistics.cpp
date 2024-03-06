#include "material/statistics.h"

Statistics::Statistics():
    totalArea(0.0),
    maxSelfEmittedRadiance(),
    maxSelfEmittedPower(),
    numberOfGeometries(0),
    numberOfCompounds(0),
    numberOfSurfaces(0),
    numberOfVertices(0),
    numberOfPatches(0),
    numberOfElements(0),
    numberOfLightSources(0),
    numberOfShadowRays(0),
    numberOfShadowCacheHits(0),
    averageDirectPotential(0.0),
    maxDirectPotential(0.0),
    maxDirectImportance(0.0),
    totalDirectPotential(0.0),
    referenceLuminance(0.0),
    totalEmittedPower(),
    estimatedAverageRadiance(),
    averageReflectivity()
{
}

Statistics GLOBAL_statistics;
