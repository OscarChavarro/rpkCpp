#ifndef __MONTE_CARLO_RADIOSITY__
#define __MONTE_CARLO_RADIOSITY__


#include "java/util/ArrayList.h"
#include "scene/Scene.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"
#include "raycasting/stochasticRaytracing/coefficientsmcrad.h"

inline int
numberOfVertices(const StochasticRadiosityElement *elem) {
    return elem->patch->numberOfVertices;
}

inline StochasticRadiosityElement*
topLevelStochasticRadiosityElement(const Patch *patch) {
    return (StochasticRadiosityElement *)patch->radianceData;
}

inline ColorRgb *
getTopLevelPatchRad(const Patch *patch) {
    return topLevelStochasticRadiosityElement(patch)->radiance;
}

inline ColorRgb *
getTopLevelPatchUnShotRad(const Patch *patch) {
    return topLevelStochasticRadiosityElement(patch)->unShotRadiance;
}

inline ColorRgb*
getTopLevelPatchReceivedRad(const Patch *patch) {
    return topLevelStochasticRadiosityElement(patch)->receivedRadiance;
}

inline GalerkinBasis *
getTopLevelPatchBasis(const Patch *patch) {
    return topLevelStochasticRadiosityElement(patch)->basis;
}

extern float monteCarloRadiosityScalarReflectance(const Patch *P);
extern void monteCarloRadiosityDefaults();
extern void monteCarloRadiosityUpdateCpuSecs();
extern Element *monteCarloRadiosityCreatePatchData(Patch *patch);
extern void monteCarloRadiosityDestroyPatchData(Patch *patch);
extern void monteCarloRadiosityPatchComputeNewColor(Patch *patch);
extern void monteCarloRadiosityInit();
extern void monteCarloRadiosityUpdateViewImportance(Scene *scene, const RenderOptions *renderOptions);
extern void monteCarloRadiosityReInit(Scene *scene, const RenderOptions *renderOptions);
extern void monteCarloRadiosityPreStep(Scene *scene, const RenderOptions *renderOptions);
extern void monteCarloRadiosityTerminate(const java::ArrayList<Patch *> *scenePatches);
extern ColorRgb monteCarloRadiosityGetRadiance(Patch *patch, double u, double v, Vector3D dir, const RenderOptions *renderOptions);
extern void doNonDiffuseFirstShot(const Scene *scene, const RadianceMethod *radianceMethod, const RenderOptions *renderOptions);

#endif
