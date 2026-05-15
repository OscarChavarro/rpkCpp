#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __RANDOM_WALK_RADIANCE_METHOD__
#define __RANDOM_WALK_RADIANCE_METHOD__

#include "scene/Camera.h"
#include "scene/RadianceMethod.h"
#include "scene/VoxelGrid.h"
#include "raycasting/stochasticRaytracing/Basismcrad.h"
#include "raycasting/stochasticRaytracing/ElementHierarchyState.h"
#include "raycasting/stochasticRaytracing/Path.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"

class RandomWalkRadianceMethod: public RadianceMethod{ public:
    explicit RandomWalkRadianceMethod( StochasticRelaxation &stochasticRelaxationState, ElementHierarchyState &elementHierarchyState, StochasticRadiosityBasisState &stochasticRadiosityBasisState);
    ~RandomWalkRadianceMethod();
    const char *getRadianceMethodName() const;
    void parseOptions(int *argc, char **argv);
    void initialize(Scene *scene, ToneMappingContext *toneMapOptions);
    bool doStep(Scene *scene, RenderOptions *renderOptions);
    void terminate(ArrayList<Patch *> *scenePatches);
    ColorRgb getRadiance(Camera *camera, Patch *patch, double u, double v, Vector3D dir, const RenderOptions *renderOptions) const;
    Element *createPatchData(Patch *patch);
    void destroyPatchData(Patch *patch);
    char *getStats() const;
    void
    writeVRML( const Camera *camera, OutputStream *outputStream, const RenderOptions *renderOptions) const;

  private:
    static const int RANDOM_WALK_STRING_LENGTH = 2000;
    StochasticRelaxation &stochasticRelaxationState;
    ElementHierarchyState &elementHierarchyState;
    StochasticRadiosityBasisState &stochasticRadiosityBasisState;

    static void appendRandomWalkStatsText(char *buffer, int *offset, const char *format, ...);
    static void randomWalkRadiosityPrintStats();
    static double randomWalkRadiosityPatchArea(const Patch *patch);
    static double rndmWalkRadSclrSrcPwr(const Patch *patch);
    static double rndmWalkRadSclrRefl(const Patch *patch);
    static ColorRgb *rndmWalkRadGetSelfEmitRadn(const StochasticRadiosityElement *elem);
    static void randomWalkRadiosityReduceSource(const ArrayList<Patch *> *scenePatches);
    static double randomWalkRadiosityScoreWeight(const Path *path, int nodeIndex);
    static void rndmWalkRadShootScr( const Path *path, long numberOfPaths, double (*birthProbability)(const Patch *patch));
    static void rndmWalkRadShootUpd(const Patch *patch, double w);
    static void rndmWalkRadDShootItrtn( const VoxelGrid *sceneWorldVoxelGrid, const ArrayList<Patch *> *scenePatches);
    static ColorRgb rndmWalkRadDetGthrnCtrlRad(const ArrayList<Patch *> *scenePatches);
    static void rndmWalkRadCllsnGthrnScr( const Path *path, long numberOfPaths, double (*birthProbability)(const Patch *patch));
    static void rndmWalkRadGthrnUpd(const Patch *patch, double w);
    static void rndmWalkRadDGthrnItrtn( const VoxelGrid *sceneWorldVoxelGrid, const ArrayList<Patch *> *scenePatches);
    static void rndmWalkRadUpdSrcIllum(StochasticRadiosityElement *elem, double w);
    static void randomWalkRadiosityDoFirstShot( VoxelGrid *sceneWorldVoxelGrid, const ArrayList<Patch *> *scenePatches, RenderOptions *renderOptions);
};

#endif
