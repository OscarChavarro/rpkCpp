#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

/**
Constant Control Radiosity
*/

#ifndef __CCR__
#define __CCR__

#include "java/util/ArrayList.h"
#include "common/color/ColorRgb.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"
#include "skin/Patch.h"

class Ccr{ public:
    typedef ColorRgb *(*GetRadianceCallback)(const StochasticRadiosityElement *);
    typedef ColorRgb (*GetScalingCallback)(StochasticRadiosityElement *);

    static ColorRgb determineControlRadiosity( GetRadianceCallback getRadiance, GetScalingCallback getScaling, const ArrayList<Patch *> *scenePatches);

  private:
    #define NUMBER_OF_INTERVALS 10
    static GetRadianceCallback getRadianceCallback;
    static GetScalingCallback getScalingCallback;

    static void initCtrlRadRec( const StochasticRadiosityElement *element, ColorRgb *minRad, ColorRgb *maxRad, ColorRgb *fMin, ColorRgb *fMax, ColorRgb *totalFluxColor, ColorRgb *maxRadColor, double *area);
    static void initialControlRadiosity( ColorRgb *minRad, ColorRgb *maxRad, ColorRgb *fMin, ColorRgb *fMax, const ArrayList<Patch *> *scenePatches);
    static void refineComponent( float *minRad, float *maxRad, float *fMin, float *fMax, const float *f, const float *rad);
    static void refineControlRadiosityRecursive( StochasticRadiosityElement *element, ColorRgb *colorOne, ColorRgb rad[NUMBER_OF_INTERVALS + 1], ColorRgb f[NUMBER_OF_INTERVALS + 1]);
    static void refineControlRadiosity( ColorRgb *minRad, ColorRgb *maxRad, ColorRgb *fMin, ColorRgb *fMax, const ArrayList<Patch *> *scenePatches);
};

#endif
