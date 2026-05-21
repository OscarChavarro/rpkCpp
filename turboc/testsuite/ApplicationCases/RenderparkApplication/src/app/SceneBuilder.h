#ifndef __SCENE_BUILDER__
#define __SCENE_BUILDER__

#include "vsdk/scene/Scene.h"
#include "vsdk/io/context/ParseRuntimeContext.h"
#include "vsdk/tonemap/ToneMappingContext.h"

class SceneBuilder{ public:
    static void sceneBuilderCreateModel( const int *argc, char *const *argv, ParseRuntimeContext *mgfContext, Scene *scene, ToneMappingContext &toneMapOptions);

  private:
    static void sceneBldPtchAccumStats(Patch *patch);
    static void sceneBuilderComputeStats(Scene *scene);
    static void sceneBldAddBgTLightSrcList(Scene *scene);
    static void sceBldAddPtcTLigSrcLisIfLigSrc(ArrayList<Patch *> *lights, Patch *patch);
    static void sceneBldFillLightSrcPtchList(Scene *scene);
    static Geometry *sceneBldCreateClustHier(const ArrayList<Patch *> *patches);
    static void sceneBuilderPatchList(const ArrayList<Geometry *> *geometryList, ArrayList<Patch *> *patchList);
    static void sceneBldFillFcsBackPntrs(const ArrayList<Geometry *> *geometryList);
    static void sceneBldCollectGeomsRec( const ArrayList<Geometry *> *source, ArrayList<Geometry *> *target);
    static void sceneBldApplyMdlTMgfCtx(ParseRuntimeContext *mgfContext, ParseSnapshotContext *mgfModel);
    static void removeEmptyMeshSurfaces(ParseRuntimeContext *mgfContext, ArrayList<Geometry *> *geometryList);
    static bool sceneBuilderHasExtension(const char *fileName, const char *extension);
    static char *sceneBldBldBnryFllbcPath(const char *mgfFileName);
    static bool sceneBldIReadRegFile(const char *fileName);
    static bool sceneBldValReadFile(const char *fileName, const char *fileRole);
    static bool sceneBuilderReadFile( const char *fileName, ParseRuntimeContext *mgfContext, Scene *scene, ToneMappingContext &toneMapOptions);
};

#endif
