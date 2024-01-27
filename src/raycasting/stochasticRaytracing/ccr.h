/**
Constant Control Radiosity
*/

#ifndef __CCR__
#define __CCR__

#include "java/util/ArrayList.h"

extern COLOR
determineControlRadiosity(
    COLOR *(*getRadiance)(StochasticRadiosityElement *),
    COLOR (*getScaling)(StochasticRadiosityElement *),
    java::ArrayList<Patch *> *scenePatches);

#endif
