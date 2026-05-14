#ifndef NONDIFF__
#define NONDIFF__

#include "vsdk/toolkit/java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgbMutable.h"
#include "vsdk/toolkit/common/linealAlgebra/Ray.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/scene/VoxelGrid.h"
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
    static Ray sampleLightRay(
        Patch *patch,
        ColorRgbMutable *emittedRadiance,
        double *pointSelectionPdf,
        double *dirSelectionPdf);
    static void sampleLight(const VoxelGrid *sceneWorldVoxelGrid, LightSourceTable *light, double lightSelectionPdf);
    static void sampleLightSources(const VoxelGrid *sceneWorldVoxelGrid, int samplesCount);
    static void summarize(const java::ArrayList<Patch *> *scenePatches);
};

#endif
