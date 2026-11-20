#ifndef __NONDIFF__
#define __NONDIFF__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Ray.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/scene/VoxelGrid.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/LightSourceTable.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"

class Nondiff{ public:
    static void doNonDiffuseFirstShot( const Scene *scene, const RadianceMethod *radianceMethod, const RenderOptions *renderOptions);

  private:
    static LightSourceTable *lights;
    static int numberOfLights;
    static int numberOfSamples;
    static double totalFlux;

    static void makeLightSourceTable( const ArrayList<Patch *> *scenePatches, const ArrayList<Patch *> *lightPatches);
    static void nextLightSample(const Patch *patch, double *zeta);
    static Ray sampleLightRay( Patch *patch, ColorRgb *emittedRadiance, double *pointSelectionPdf, double *dirSelectionPdf);
    static void sampleLight(const VoxelGrid *sceneWorldVoxelGrid, VoxelGridWorkspace *sceneWorldVoxelGridWorkspace, LightSourceTable *light, double lightSelectionPdf);
    static void sampleLightSources(const VoxelGrid *sceneWorldVoxelGrid, VoxelGridWorkspace *sceneWorldVoxelGridWorkspace, int samplesCount);
    static void summarize(const ArrayList<Patch *> *scenePatches);
};

#endif
