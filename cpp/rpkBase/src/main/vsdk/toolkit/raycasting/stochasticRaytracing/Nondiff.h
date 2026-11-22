#ifndef NONDIFF__
#define NONDIFF__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "vsdk/toolkit/common/linealAlgebra/RpkRay.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/scene/SceneQuery.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/LightSourceTable.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

class Nondiff final {
  public:
    static void doNonDiffuseFirstShot(
        const Scene *scene,
        const RadianceMethod *radianceMethod,
        const RendererConfiguration *renderOptions);

  private:
    static LightSourceTable *lights;
    static int numberOfLights;
    static int numberOfSamples;
    static double totalFlux;

    static void makeLightSourceTable(
        const java::ArrayList<Patch *> *scenePatches,
        const java::ArrayList<Patch *> *lightPatches);
    static void nextLightSample(const Patch *patch, double *zeta);
    static RpkRay sampleLightRay(
        Patch *patch,
        ColorRgbMutable *emittedRadiance,
        double *pointSelectionPdf,
        double *dirSelectionPdf);
    static void sampleLight(const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneWorldSceneQueryWorkspace, LightSourceTable *light, double lightSelectionPdf);
    static void sampleLightSources(const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneWorldSceneQueryWorkspace, int samplesCount);
    static void summarize(const java::ArrayList<Patch *> *scenePatches, FaceColorTable *faceColors);
};

#endif
