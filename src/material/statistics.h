#ifndef _STATISTICS_H_
#define _STATISTICS_H_

#include "material/color.h"

/* general statistics about the current scene */
extern int GLOBAL_statistics_numberOfGeometries;
extern int GLOBAL_statistics_numberOfCompounds;
extern int GLOBAL_statistics_numberOfSurfaces;
extern int GLOBAL_statistics_numberOfVertices;
extern int GLOBAL_statistics_numberOfPatches;
extern int GLOBAL_statistics_numberOfElements;
extern int GLOBAL_statistics_numberOfLightSources;
extern int GLOBAL_statistics_numberOfShadowRays;
extern int GLOBAL_statistics_numberOfShadowCacheHits;
extern float GLOBAL_statistics_totalArea;
extern double GLOBAL_statistics_averageDirectPotential;
extern double GLOBAL_statistics_maxDirectPotential;
extern double GLOBAL_statistics_maxDirectImportance; // potential times area
extern double GLOBAL_statistics_totalDirectPotential;
extern double GLOBAL_statistics_referenceLuminance;
extern COLOR GLOBAL_statistics_totalEmittedPower;
extern COLOR GLOBAL_statistics_estimatedAverageRadiance;
extern COLOR GLOBAL_statistics_averageReflectivity;
extern COLOR GLOBAL_statistics_maxSelfEmittedRadiance;
extern COLOR GLOBAL_statistics_maxSelfEmittedPower;

#endif
