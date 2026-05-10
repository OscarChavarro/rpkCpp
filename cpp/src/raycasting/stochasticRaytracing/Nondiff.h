#ifndef __NONDIFF__
#define __NONDIFF__

#include "java/util/ArrayList.h"
#include "common/color/ColorRgb.h"
#include "common/linealAlgebra/Ray.h"
#include "material/RendererConfiguration.h"
#include "scene/RadianceMethod.h"
#include "scene/Scene.h"
#include "scene/VoxelGrid.h"
#include "raycasting/stochasticRaytracing/LightSourceTable.h"
#include "environment/geometry/elements/Patch.h"

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
    static Ray sampleLightRay(
        Patch *patch,
        ColorRgb *emittedRadiance,
        double *pointSelectionPdf,
        double *dirSelectionPdf);
    static void sampleLight(const VoxelGrid *sceneWorldVoxelGrid, LightSourceTable *light, double lightSelectionPdf);
    static void sampleLightSources(const VoxelGrid *sceneWorldVoxelGrid, int samplesCount);
    static void summarize(const java::ArrayList<Patch *> *scenePatches);
};

#endif
