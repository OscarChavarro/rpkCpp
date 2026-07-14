#ifndef RANDOM_WALK_RADIANCE_METHOD__
#define RANDOM_WALK_RADIANCE_METHOD__

#include "vsdk/toolkit/scene/Camera.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/scene/VoxelGrid.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Basismcrad.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/ElementHierarchyState.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Path.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRadiosityElement.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRelaxation.h"

class RandomWalkRadianceMethod final : public RadianceMethod {
  public:
    explicit RandomWalkRadianceMethod(
        StochasticRelaxation &stochasticRelaxationState,
        ElementHierarchyState &elementHierarchyState,
        StochasticRadiosityBasisState &stochasticRadiosityBasisState);
    ~RandomWalkRadianceMethod() final;
    const char *getRadianceMethodName() const final;
    void parseOptions(int *argc, char **argv) final;
    void initialize(Scene *scene, ToneMappingContext *toneMapOptions) final;
    bool doStep(Scene *scene, RendererConfiguration *renderOptions) final;
    void terminate(java::ArrayList<Patch *> *scenePatches) final;
    ColorRgbMutable getRadiance(Camera *camera, Patch *patch, double u, double v, Vector3D dir, const RendererConfiguration *renderOptions) const final;
    Element *createPatchData(Patch *patch) final;
    void destroyPatchData(Patch *patch) final;
    char *getStats() const final;
    void
    writeVRML(
        const Camera *camera,
        java::OutputStream *outputStream,
        const RendererConfiguration *renderOptions) const final;

  private:
    static constexpr int STRING_LENGTH = 2000;
    StochasticRelaxation &stochasticRelaxationState;
    ElementHierarchyState &elementHierarchyState;
    StochasticRadiosityBasisState &stochasticRadiosityBasisState;

    static void appendRandomWalkStatsText(char *buffer, int *offset, const char *format, ...);
    static void randomWalkRadiosityPrintStats();
    static double randomWalkRadiosityPatchArea(const Patch *patch);
    static double randomWalkRadiosityScalarSourcePower(const Patch *patch);
    static double randomWalkRadiosityScalarReflectance(const Patch *patch);
    static ColorRgbMutable *randomWalkRadiosityGetSelfEmittedRadiance(const StochasticRadiosityElement *elem);
    static void randomWalkRadiosityReduceSource(const java::ArrayList<Patch *> *scenePatches);
    static double randomWalkRadiosityScoreWeight(const Path *path, int nodeIndex);
    static void randomWalkRadiosityShootingScore(
        const Path *path,
        long numberOfPaths,
        double (*birthProbability)(const Patch *patch));
    static void randomWalkRadiosityShootingUpdate(const Patch *patch, double w);
    static void randomWalkRadiosityDoShootingIteration(
        const VoxelGrid *sceneWorldVoxelGrid,
        const java::ArrayList<Patch *> *scenePatches);
    static ColorRgbMutable randomWalkRadiosityDetermineGatheringControlRadiosity(const java::ArrayList<Patch *> *scenePatches);
    static void randomWalkRadiosityCollisionGatheringScore(
        const Path *path,
        long numberOfPaths,
        double (*birthProbability)(const Patch *patch));
    static void randomWalkRadiosityGatheringUpdate(const Patch *patch, double w);
    static void randomWalkRadiosityDoGatheringIteration(
        const VoxelGrid *sceneWorldVoxelGrid,
        const java::ArrayList<Patch *> *scenePatches);
    static void randomWalkRadiosityUpdateSourceIllumination(StochasticRadiosityElement *elem, double w);
    static void randomWalkRadiosityDoFirstShot(
        VoxelGrid *sceneWorldVoxelGrid,
        const java::ArrayList<Patch *> *scenePatches,
        RendererConfiguration *renderOptions);
};

#endif
