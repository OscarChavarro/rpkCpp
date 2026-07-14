/**
Constant Control Radiosity
*/

#ifndef CCR__
#define CCR__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRadiosityElement.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

class Ccr final {
  public:
    using GetRadianceCallback = ColorRgbMutable *(*)(const StochasticRadiosityElement *);
    using GetScalingCallback = ColorRgbMutable (*)(StochasticRadiosityElement *);

    static ColorRgbMutable determineControlRadiosity(
        GetRadianceCallback getRadiance,
        GetScalingCallback getScaling,
        const java::ArrayList<Patch *> *scenePatches);

  private:
    static constexpr int NUMBER_OF_INTERVALS = 10;
    static GetRadianceCallback getRadianceCallback;
    static GetScalingCallback getScalingCallback;

    static void initialControlRadiosityRecursive(
        const StochasticRadiosityElement *element,
        ColorRgbMutable *minRad,
        ColorRgbMutable *maxRad,
        ColorRgbMutable *fMin,
        ColorRgbMutable *fMax,
        ColorRgbMutable *totalFluxColor,
        ColorRgbMutable *maxRadColor,
        double *area);
    static void initialControlRadiosity(
        ColorRgbMutable *minRad,
        ColorRgbMutable *maxRad,
        ColorRgbMutable *fMin,
        ColorRgbMutable *fMax,
        const java::ArrayList<Patch *> *scenePatches);
    static void refineComponent(
        double *minRad,
        double *maxRad,
        double *fMin,
        double *fMax,
        const double *f,
        const double *rad);
    static void refineControlRadiosityRecursive(
        StochasticRadiosityElement *element,
        ColorRgbMutable *colorOne,
        ColorRgbMutable rad[NUMBER_OF_INTERVALS + 1],
        ColorRgbMutable f[NUMBER_OF_INTERVALS + 1]);
    static void refineControlRadiosity(
        ColorRgbMutable *minRad,
        ColorRgbMutable *maxRad,
        ColorRgbMutable *fMin,
        ColorRgbMutable *fMax,
        const java::ArrayList<Patch *> *scenePatches);
};

#endif
