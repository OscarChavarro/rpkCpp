#include "material/statistics.h"

int GLOBAL_statistics_numberOfGeometries = 0;
int GLOBAL_statistics_numberOfCompounds = 0;
int GLOBAL_statistics_numberOfSurfaces = 0;
int GLOBAL_statistics_numberOfVertices = 0;
int GLOBAL_statistics_numberOfPatches = 0;
int GLOBAL_statistics_numberOfElements = 0;
int GLOBAL_statistics_numberOfLightSources = 0;
int GLOBAL_statistics_numberOfShadowRays = 0;
int GLOBAL_statistics_numberOfShadowCacheHits = 0;
float GLOBAL_statistics_totalArea = 0.0;
double GLOBAL_statistics_averageDirectPotential = 0.0;
double GLOBAL_statistics_maxDirectPotential = 0.0;
double GLOBAL_statistics_maxDirectImportance = 0.0;
double GLOBAL_statistics_totalDirectPotential = 0.0;
double GLOBAL_statistics_referenceLuminance = 0.0;
COLOR GLOBAL_statistics_totalEmittedPower;
COLOR GLOBAL_statistics_estimatedAverageRadiance;
COLOR GLOBAL_statistics_averageReflectivity;
COLOR GLOBAL_statistics_maxSelfEmittedRadiance;
COLOR GLOBAL_statistics_maxSelfEmittedPower;

Statistics GLOBAL_statistics;
