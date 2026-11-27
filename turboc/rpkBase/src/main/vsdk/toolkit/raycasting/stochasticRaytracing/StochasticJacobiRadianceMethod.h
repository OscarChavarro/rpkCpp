#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef STCHS_JCB_RDSTY_MTHD
#define STCHS_JCB_RDSTY_MTHD

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/scene/SceneQuery.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Basismcrad.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/ElementHierarchyState.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRadiosityElement.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRelaxation.h"

class StochasticJacobiRadianceMethod: public RadianceMethod{ public:
    explicit StochasticJacobiRadianceMethod( StochasticRelaxation &stochasticRelaxationState, ElementHierarchyState &elementHierarchyState, StochasticRadiosityBasisState &stochasticRadiosityBasisState);
    ~StochasticJacobiRadianceMethod();
    const char *getRadianceMethodName() const;
    void parseOptions(int *argc, char **argv);
    void initialize(Scene *scene, ToneMappingContext *toneMapOptions);
    bool doStep(Scene *scene, RenderOptions *renderOptions);
    void terminate(ArrayList<Patch *> *scenePatches);
    ColorRgbMutable getRadiance(Camera *camera, Patch *patch, double u, double v, Vector3Dd dir, const RenderOptions *renderOptions) const;
    Element *createPatchData(Patch *patch);
    void destroyPatchData(Patch *patch);
    char *getStats() const;
    void
    writeVRML( const Camera *camera, OutputStream *outputStream, const RenderOptions *renderOptions) const;

  private:
    static const int STOCHASTIC_JACOBI_STRING_LENGTH = 2000;
    StochasticRelaxation &stochasticRelaxationState;
    ElementHierarchyState &elementHierarchyState;
    StochasticRadiosityBasisState &stochasticRadiosityBasisState;

    static void appendStochasticStatsText(char *buffer, int *offset, const char *format, ...);
    static long stchsRelaxRadRndRnd(double x);
    static void stchsRelaxRadRecompDispClrs(const ArrayList<Patch *> *scenePatches);
    static double stchsRelaxRadQualFactor(const StochasticRadiosityElement *elem, double w);
    static ColorRgbMutable *stchsRelaxRadElemUnShotRadn(const StochasticRadiosityElement *elem);
    static void stchsRelaxRadElemIncrRadn(StochasticRadiosityElement *elem, double w);
    static void stchRelaRadPrinIncrRadnStat();
    static void stchsRelaxRadDIncrmRadnItrtn( Scene *scene, const RadianceMethod *radianceMethod, RenderOptions *renderOptions);
    static double stchsRelaxRadElemUnShotImp(const StochasticRadiosityElement *elem);
    static void stchsRelaxRadElemIncrImp(StochasticRadiosityElement *elem, double w);
    static void stchsRelaxRadPrintIncrmImpStats();
    static void stchsRelaxRadDIncrmImpItrtn( const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneWorldSceneQueryWorkspace, const ArrayList<Patch *> *scenePatches, RenderOptions *renderOptions);
    static ColorRgbMutable *stchsRelaxRadElemRadn(const StochasticRadiosityElement *elem);
    static void stchsRelaxRadElemUpdRadn(StochasticRadiosityElement *elem, double w);
    static void stchsRelaxRadPrintRegStats();
    static void stchsRelaxRadDRegRadnItrtn( const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneWorldSceneQueryWorkspace, const ArrayList<Patch *> *scenePatches, RenderOptions *renderOptions);
    static double stchsRelaxRadElemImp(const StochasticRadiosityElement *elem);
    static void stchsRelaxRadElemUpdImp(StochasticRadiosityElement *elem, double w);
    static void stchsRelaxRadDRegImpItrtn( const SceneQuery *sceneQuery, SceneQueryWorkspace *sceneWorldSceneQueryWorkspace, const ArrayList<Patch *> *scenePatches, RenderOptions *renderOptions);
    static void stchsRelaxRadElemDscrdIncrm(Element *element);
    static void stchsRelaxRadDscrdIncrm();
};

#endif
