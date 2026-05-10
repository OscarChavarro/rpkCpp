#ifndef __STOCHASTIC_JACOBI_RADIOSITY_METHOD__
#define __STOCHASTIC_JACOBI_RADIOSITY_METHOD__

#include "java/util/ArrayList.h"
#include "scene/RadianceMethod.h"
#include "scene/VoxelGrid.h"
#include "raycasting/stochasticRaytracing/Basismcrad.h"
#include "raycasting/stochasticRaytracing/ElementHierarchyState.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"

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
    ColorRgb getRadiance(Camera *camera, Patch *patch, double u, double v, Vector3D dir, const RendererConfiguration *renderOptions) const final;
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
    static long stochasticRelaxationRadiosityRandomRound(float x);
    static void stochasticRelaxationRadiosityRecomputeDisplayColors(const java::ArrayList<Patch *> *scenePatches);
    static double stochasticRelaxationRadiosityQualityFactor(const StochasticRadiosityElement *elem, double w);
    static ColorRgb *stochasticRelaxationRadiosityElementUnShotRadiance(const StochasticRadiosityElement *elem);
    static void stochasticRelaxationRadiosityElementIncrementRadiance(StochasticRadiosityElement *elem, double w);
    static void stochasticRelaxationRadiosityPrintIncrementalRadianceStats();
    static void stochasticRelaxationRadiosityDoIncrementalRadianceIterations(
        Scene *scene,
        const RadianceMethod *radianceMethod,
        RendererConfiguration *renderOptions);
    static float stochasticRelaxationRadiosityElementUnShotImportance(const StochasticRadiosityElement *elem);
    static void stochasticRelaxationRadiosityElementIncrementImportance(StochasticRadiosityElement *elem, double w);
    static void stochasticRelaxationRadiosityPrintIncrementalImportanceStats();
    static void stochasticRelaxationRadiosityDoIncrementalImportanceIterations(
        VoxelGrid *sceneWorldVoxelGrid,
        const java::ArrayList<Patch *> *scenePatches,
        RendererConfiguration *renderOptions);
    static ColorRgb *stochasticRelaxationRadiosityElementRadiance(const StochasticRadiosityElement *elem);
    static void stochasticRelaxationRadiosityElementUpdateRadiance(StochasticRadiosityElement *elem, double w);
    static void stochasticRelaxationRadiosityPrintRegularStats();
    static void stochasticRelaxationRadiosityDoRegularRadianceIteration(
        VoxelGrid *sceneWorldVoxelGrid,
        const java::ArrayList<Patch *> *scenePatches,
        RendererConfiguration *renderOptions);
    static float stochasticRelaxationRadiosityElementImportance(const StochasticRadiosityElement *elem);
    static void stochasticRelaxationRadiosityElementUpdateImportance(StochasticRadiosityElement *elem, double w);
    static void stochasticRelaxationRadiosityDoRegularImportanceIteration(
        VoxelGrid *sceneWorldVoxelGrid,
        const java::ArrayList<Patch *> *scenePatches,
        RendererConfiguration *renderOptions);
    static void stochasticRelaxationRadiosityElementDiscardIncremental(Element *element);
    static void stochasticRelaxationRadiosityDiscardIncremental();
};

#endif
