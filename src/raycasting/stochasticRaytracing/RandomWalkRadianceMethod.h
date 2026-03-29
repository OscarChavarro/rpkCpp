#ifndef __RANDOM_WALK_RADIANCE_METHOD__
#define __RANDOM_WALK_RADIANCE_METHOD__

#include "scene/Camera.h"
#include "scene/RadianceMethod.h"

class Path;
class StochasticRadiosityElement;
class VoxelGrid;

class RandomWalkRadianceMethod final : public RadianceMethod {
  public:
    RandomWalkRadianceMethod();
    ~RandomWalkRadianceMethod() final;
    const char *getRadianceMethodName() const final;
    void parseOptions(int *argc, char **argv) final;
    void initialize(Scene *scene) final;
    bool doStep(Scene *scene, RenderOptions *renderOptions) final;
    void terminate(java::ArrayList<Patch *> *scenePatches) final;
    ColorRgb getRadiance(Camera *camera, Patch *patch, double u, double v, Vector3D dir, const RenderOptions *renderOptions) const final;
    Element *createPatchData(Patch *patch) final;
    void destroyPatchData(Patch *patch) final;
    char *getStats() final;
    void renderScene(const Scene *scene, const RenderOptions *renderOptions) const final;
    void
    writeVRML(
        const Camera *camera,
        java::io::OutputStream *outputStream,
        const RenderOptions *renderOptions) const final;

  private:
    static void appendRandomWalkStatsText(char *buffer, int *offset, const char *format, ...);
    static void randomWalkRadiosityPrintStats();
    static double randomWalkRadiosityPatchArea(const Patch *patch);
    static double randomWalkRadiosityScalarSourcePower(const Patch *patch);
    static double randomWalkRadiosityScalarReflectance(const Patch *patch);
    static ColorRgb *randomWalkRadiosityGetSelfEmittedRadiance(const StochasticRadiosityElement *elem);
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
    static ColorRgb randomWalkRadiosityDetermineGatheringControlRadiosity(const java::ArrayList<Patch *> *scenePatches);
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
        RenderOptions *renderOptions);
};

#endif
