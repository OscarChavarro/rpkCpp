/**
Constant Control Radiosity
*/

#ifndef CCR__
#define CCR__

#include "java/util/ArrayList.h"
#include "common/color/ColorRgb.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"
#include "environment/geometry/elements/Patch.h"

class Ccr final {
  public:
    using GetRadianceCallback = ColorRgb *(*)(const StochasticRadiosityElement *);
    using GetScalingCallback = ColorRgb (*)(StochasticRadiosityElement *);

    static ColorRgb determineControlRadiosity(
        GetRadianceCallback getRadiance,
        GetScalingCallback getScaling,
        const java::ArrayList<Patch *> *scenePatches);

  private:
    static constexpr int NUMBER_OF_INTERVALS = 10;
    static GetRadianceCallback getRadianceCallback;
    static GetScalingCallback getScalingCallback;

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
        ColorRgb rad[NUMBER_OF_INTERVALS + 1],
        ColorRgb f[NUMBER_OF_INTERVALS + 1]);
    static void refineControlRadiosity(
        ColorRgb *minRad,
        ColorRgb *maxRad,
        ColorRgb *fMin,
        ColorRgb *fMax,
        const java::ArrayList<Patch *> *scenePatches);
};

#endif
