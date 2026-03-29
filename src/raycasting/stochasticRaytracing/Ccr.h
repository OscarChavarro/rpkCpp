/**
Constant Control Radiosity
*/

#ifndef __CCR__
#define __CCR__

#include "java/util/ArrayList.h"
#include "common/ColorRgb.h"

class StochasticRadiosityElement;
class Patch;

class Ccr final {
  public:
    static ColorRgb determineControlRadiosity(
        ColorRgb *(*getRadiance)(const StochasticRadiosityElement *),
        ColorRgb (*getScaling)(StochasticRadiosityElement *),
        const java::ArrayList<Patch *> *scenePatches);

  private:
    static void initialControlRadiosityRecursive(
        const StochasticRadiosityElement *element,
        ColorRgb *minRad,
        ColorRgb *maxRad,
        ColorRgb *fMin,
        ColorRgb *fMax,
        ColorRgb *totalFluxColor,
        ColorRgb *maxRadColor,
        double *area);
    static void initialControlRadiosity(
        ColorRgb *minRad,
        ColorRgb *maxRad,
        ColorRgb *fMin,
        ColorRgb *fMax,
        const java::ArrayList<Patch *> *scenePatches);
    static void refineComponent(
        float *minRad,
        float *maxRad,
        float *fMin,
        float *fMax,
        const float *f,
        const float *rad);
    static void refineControlRadiosityRecursive(
        StochasticRadiosityElement *element,
        ColorRgb *colorOne,
        ColorRgb rad[11],
        ColorRgb f[11]);
    static void refineControlRadiosity(
        ColorRgb *minRad,
        ColorRgb *maxRad,
        ColorRgb *fMin,
        ColorRgb *fMax,
        const java::ArrayList<Patch *> *scenePatches);
};

#endif
