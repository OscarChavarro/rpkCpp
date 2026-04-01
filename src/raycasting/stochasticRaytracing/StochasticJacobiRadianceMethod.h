#ifndef __STOCHASTIC_JACOBI_RADIOSITY_METHOD__
#define __STOCHASTIC_JACOBI_RADIOSITY_METHOD__

#include "java/util/ArrayList.h"
#include "scene/RadianceMethod.h"

class StochasticRadiosityElement;
class VoxelGrid;

class StochasticJacobiRadianceMethod final : public RadianceMethod {
  public:
    StochasticJacobiRadianceMethod();
    ~StochasticJacobiRadianceMethod() final;
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
        RenderOptions *renderOptions);
    static float stochasticRelaxationRadiosityElementUnShotImportance(const StochasticRadiosityElement *elem);
    static void stochasticRelaxationRadiosityElementIncrementImportance(StochasticRadiosityElement *elem, double w);
    static void stochasticRelaxationRadiosityPrintIncrementalImportanceStats();
    static void stochasticRelaxationRadiosityDoIncrementalImportanceIterations(
        VoxelGrid *sceneWorldVoxelGrid,
        const java::ArrayList<Patch *> *scenePatches,
        RenderOptions *renderOptions);
    static ColorRgb *stochasticRelaxationRadiosityElementRadiance(const StochasticRadiosityElement *elem);
    static void stochasticRelaxationRadiosityElementUpdateRadiance(StochasticRadiosityElement *elem, double w);
    static void stochasticRelaxationRadiosityPrintRegularStats();
    static void stochasticRelaxationRadiosityDoRegularRadianceIteration(
        VoxelGrid *sceneWorldVoxelGrid,
        const java::ArrayList<Patch *> *scenePatches,
        RenderOptions *renderOptions);
    static float stochasticRelaxationRadiosityElementImportance(const StochasticRadiosityElement *elem);
    static void stochasticRelaxationRadiosityElementUpdateImportance(StochasticRadiosityElement *elem, double w);
    static void stochasticRelaxationRadiosityDoRegularImportanceIteration(
        VoxelGrid *sceneWorldVoxelGrid,
        const java::ArrayList<Patch *> *scenePatches,
        RenderOptions *renderOptions);
    static void stochasticRelaxationRadiosityElementDiscardIncremental(Element *element);
    static void stochasticRelaxationRadiosityDiscardIncremental();
};

#endif
