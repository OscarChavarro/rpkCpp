/**
Monte Carlo radiosity
*/

#ifndef _MONTE_CARLO_RADIOSITY__
#define _MONTE_CARLO_RADIOSITY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/environment/geometry/elements/Element.h"
#include "vsdk/toolkit/skin/Geometry.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/Vertex.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"

class StochasticRadiosityElement;

class Mcrad final {
  public:
    static float monteCarloRadiosityScalarReflectance(const Patch *patch);
    static void monteCarloRadiosityDefaults();
    static void monteCarloRadiosityUpdateCpuSecs();
    static Element *monteCarloRadiosityCreatePatchData(Patch *patch);
    static void monteCarloRadiosityDestroyPatchData(Patch *patch);
    static void monteCarloRadiosityPatchComputeNewColor(Patch *patch);
    static void monteCarloRadiosityInit();
    static void monteCarloRadiosityUpdateViewImportance(Scene *scene, const RendererConfiguration *renderOptions);
    static void monteCarloRadiosityReInit(Scene *scene, const RendererConfiguration *renderOptions);
    static void monteCarloRadiosityPreStep(Scene *scene, const RendererConfiguration *renderOptions);
    static void monteCarloRadiosityTerminate(const java::ArrayList<Patch *> *scenePatches);
    static ColorRgbMutable monteCarloRadiosityGetRadiance(
        Patch *patch,
        double u,
        double v,
        Vector3D dir,
        const RendererConfiguration *renderOptions);

  private:
    static void monteCarloRadiosityInitPatch(const Patch *patch);
    static void monteCarloRadiosityPullImportances(Element *element);
    static void monteCarloRadiosityAccumulateImportances(const StochasticRadiosityElement *elem);
    static void monteCarloRadiosityUpdateImportance(Element *element);
    static void monteCarloRadiosityReInitImportance(Element *element);
    static double monteCarloRadiosityDetermineAreaFraction(
        const java::ArrayList<Patch *> *scenePatches,
        const java::ArrayList<Geometry *> *sceneGeometries);
    static void monteCarloRadiosityDetermineInitialNrRays(
        const java::ArrayList<Patch *> *scenePatches,
        const java::ArrayList<Geometry *> *sceneGeometries);
    static ColorRgbMutable monteCarloRadiosityDiffuseReflectanceAtPoint(Patch *patch, double u, double v);
    static ColorRgbMutable vertexReflectance(const Vertex *vertex);
    static ColorRgbMutable monteCarloRadiosityInterpolatedReflectanceAtPoint(
        const StochasticRadiosityElement *leaf,
        double u,
        double v);
};

#endif
