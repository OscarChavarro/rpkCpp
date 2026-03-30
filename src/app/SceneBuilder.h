#ifndef __SCENE_BUILDER__
#define __SCENE_BUILDER__

#include "scene/Scene.h"
#include "io/context/ParseSession.h"

class ToneMappingContext;

class SceneBuilder final {
  public:
    static void sceneBuilderCreateModel(
        const int *argc,
        char *const *argv,
        ParseSession *mgfContext,
        Scene *scene,
        ToneMappingContext &toneMapOptions);

  private:
    static void sceneBuilderPatchAccumulateStats(Patch *patch);
    static void sceneBuilderComputeStats(Scene *scene);
    static void sceneBuilderAddBackgroundToLightSourceList(Scene *scene);
    static void sceneBuilderAddPatchToLightSourceListIfLightSource(java::ArrayList<Patch *> *lights, Patch *patch);
    static void sceneBuilderFillLightSourcePatchList(Scene *scene);
    static Geometry *sceneBuilderCreateClusterHierarchy(const java::ArrayList<Patch *> *patches);
    static void sceneBuilderPatchList(const java::ArrayList<Geometry *> *geometryList, java::ArrayList<Patch *> *patchList);
    static void sceneBuilderFillFacesBackPointers(const java::ArrayList<Geometry *> *geometryList);
    static void sceneBuilderCollectGeometriesRecursive(
        const java::ArrayList<Geometry *> *source,
        java::ArrayList<Geometry *> *target);
    static void sceneBuilderApplyModelToMgfContext(ParseSession *mgfContext, PersistedSceneModel *mgfModel);
    static void removeEmptyMeshSurfaces(ParseSession *mgfContext, java::ArrayList<Geometry *> *geometryList);
    static bool sceneBuilderValidateReadableFile(const char *fileName, const char *fileRole);
    static bool sceneBuilderReadFile(
        const char *fileName,
        ParseSession *mgfContext,
        Scene *scene,
        ToneMappingContext &toneMapOptions);
};

#endif
