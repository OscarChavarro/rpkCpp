#ifndef __STATISTICS__
#define __STATISTICS__

#include "common/color.h"

class Statistics {
  public:
    float totalArea;
    COLOR maxSelfEmittedRadiance;
    COLOR maxSelfEmittedPower;
    int numberOfGeometries;
    int numberOfCompounds;
    int numberOfSurfaces;
    int numberOfVertices;
    int numberOfPatches;
    int numberOfElements;
    int numberOfLightSources;
    int numberOfShadowRays;
    int numberOfShadowCacheHits;
    double averageDirectPotential;
    double maxDirectPotential;
    double maxDirectImportance; // Potential times area
    double totalDirectPotential;
    double referenceLuminance;
    COLOR totalEmittedPower;
    COLOR estimatedAverageRadiance;
    COLOR averageReflectivity;

    Statistics();
};

// General statistics about the current scene
extern Statistics GLOBAL_statistics;

#endif
