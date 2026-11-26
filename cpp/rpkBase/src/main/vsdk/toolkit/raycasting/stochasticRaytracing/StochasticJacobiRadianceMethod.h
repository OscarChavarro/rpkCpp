#ifndef __STOCHASTIC_JACOBI_RADIANCE_METHOD__
#define __STOCHASTIC_JACOBI_RADIANCE_METHOD__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/scene/SceneQuery.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Basismcrad.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/ElementHierarchyState.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRadiosityElement.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRelaxation.h"

class StochasticJacobiRadianceMethod final : public RadianceMethod {
  public:
    explicit StochasticJacobiRadianceMethod(
        StochasticRelaxation &stochasticRelaxationState,
        ElementHierarchyState &elementHierarchyState,
        StochasticRadiosityBasisState &stochasticRadiosityBasisState);
    ~StochasticJacobiRadianceMethod() final;
    const char *getRadianceMethodName() const final;
    void parseOptions(int *argc, char **argv) final;
    void initialize(Scene *scene, ToneMappingContext *toneMapOptions) final;
    bool doStep(Scene *scene, RendererConfiguration *renderOptions) final;
    void terminate(java::ArrayList<Patch *> *scenePatches) final;
    ColorRgb getRadiance(Camera *camera, const Patch *patch, double u, double v, Vector3Dd dir, const RendererConfiguration *renderOptions) const final;
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

    static void appendStochasticStatsText(char *buffer, int *offset, const char *format, ...);
    static long stochasticRelaxationRadiosityRandomRound(double x);
    static void stochasticRelaxationRadiosityRecomputeDisplayColors(const java::ArrayList<Patch *> *scenePatches, FaceColorTable *faceColors);
    static double stochasticRelaxationRadiosityQualityFactor(const StochasticRadiosityElement *elem, double w);
    static ColorRgb *stochasticRelaxationRadiosityElementUnShotRadiance(const StochasticRadiosityElement *elem);
    static void stochasticRelaxationRadiosityElementIncrementRadiance(StochasticRadiosityElement *elem, double w);
    static void stochasticRelaxationRadiosityPrintIncrementalRadianceStats();
    static void stochasticRelaxationRadiosityDoIncrementalRadianceIterations(
        Scene *scene,
        const RadianceMethod *radianceMethod,
        RendererConfiguration *renderOptions);
    static double stochasticRelaxationRadiosityElementUnShotImportance(const StochasticRadiosityElement *elem);
    static void stochasticRelaxationRadiosityElementIncrementImportance(StochasticRadiosityElement *elem, double w);
    static void stochasticRelaxationRadiosityPrintIncrementalImportanceStats();
    static void stochasticRelaxationRadiosityDoIncrementalImportanceIterations(
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneWorldSceneQueryWorkspace,
        const java::ArrayList<Patch *> *scenePatches,
        RendererConfiguration *renderOptions);
    static ColorRgb *stochasticRelaxationRadiosityElementRadiance(const StochasticRadiosityElement *elem);
    static void stochasticRelaxationRadiosityElementUpdateRadiance(StochasticRadiosityElement *elem, double w);
    static void stochasticRelaxationRadiosityPrintRegularStats();
    static void stochasticRelaxationRadiosityDoRegularRadianceIteration(
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneWorldSceneQueryWorkspace,
        const java::ArrayList<Patch *> *scenePatches,
        RendererConfiguration *renderOptions);
    static double stochasticRelaxationRadiosityElementImportance(const StochasticRadiosityElement *elem);
    static void stochasticRelaxationRadiosityElementUpdateImportance(StochasticRadiosityElement *elem, double w);
    static void stochasticRelaxationRadiosityDoRegularImportanceIteration(
        const SceneQuery *sceneQuery,
        SceneQueryWorkspace *sceneWorldSceneQueryWorkspace,
        const java::ArrayList<Patch *> *scenePatches,
        RendererConfiguration *renderOptions);
    static void stochasticRelaxationRadiosityElementDiscardIncremental(Element *element);
    static void stochasticRelaxationRadiosityDiscardIncremental();
};

#endif
