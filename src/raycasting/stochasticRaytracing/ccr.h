/**
Constant Control Radiosity
*/

#ifndef __CCR__
#define __CCR__

#include "java/util/ArrayList.h"

extern ColorRgb
determineControlRadiosity(
        ColorRgb *(*getRadiance)(StochasticRadiosityElement *),
        ColorRgb (*getScaling)(StochasticRadiosityElement *),
        java::ArrayList<Patch *> *scenePatches);

#endif
