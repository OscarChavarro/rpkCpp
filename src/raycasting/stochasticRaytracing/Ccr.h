/**
Constant Control Radiosity
*/

#ifndef __CCR__
#define __CCR__

#include "java/util/ArrayList.h"

extern ColorRgb
determineControlRadiosity(
    ColorRgb *(*getRadiance)(const StochasticRadiosityElement *),
    ColorRgb (*getScaling)(StochasticRadiosityElement *),
    const java::ArrayList<Patch *> *scenePatches);

#endif
