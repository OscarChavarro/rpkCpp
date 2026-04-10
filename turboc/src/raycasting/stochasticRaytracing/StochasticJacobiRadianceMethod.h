#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef STCHS_JCB_RDSTY_MTHD
#define STCHS_JCB_RDSTY_MTHD

#include "java/util/ArrayList.h"
#include "scene/RadianceMethod.h"
#include "scene/VoxelGrid.h"
#include "raycasting/stochasticRaytracing/Basismcrad.h"
#include "raycasting/stochasticRaytracing/ElementHierarchyState.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"
#include "raycasting/stochasticRaytracing/StochasticRelaxation.h"

class StochasticJacobiRadianceMethod: public RadianceMethod{ public:
    explicit StochasticJacobiRadianceMethod( StochasticRelaxation &stochasticRelaxationState, ElementHierarchyState &elementHierarchyState, StochasticRadiosityBasisState &stochasticRadiosityBasisState);
    ~StochasticJacobiRadianceMethod();
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
    static const int STOCHASTIC_JACOBI_STRING_LENGTH = 2000;
    StochasticRelaxation &stochasticRelaxationState;
    ElementHierarchyState &elementHierarchyState;
    StochasticRadiosityBasisState &stochasticRadiosityBasisState;

    static void appendStochasticStatsText(char *buffer, int *offset, const char *format, ...);
    static long stchsRelaxRadRndRnd(float x);
    static void stchsRelaxRadRecompDispClrs(const ArrayList<Patch *> *scenePatches);
    static double stchsRelaxRadQualFactor(const StochasticRadiosityElement *elem, double w);
    static ColorRgb *stchsRelaxRadElemUnShotRadn(const StochasticRadiosityElement *elem);
    static void stchsRelaxRadElemIncrRadn(StochasticRadiosityElement *elem, double w);
    static void stchRelaRadPrinIncrRadnStat();
    static void stchsRelaxRadDIncrmRadnItrtn( Scene *scene, const RadianceMethod *radianceMethod, RenderOptions *renderOptions);
    static float stchsRelaxRadElemUnShotImp(const StochasticRadiosityElement *elem);
    static void stchsRelaxRadElemIncrImp(StochasticRadiosityElement *elem, double w);
    static void stchsRelaxRadPrintIncrmImpStats();
    static void stchsRelaxRadDIncrmImpItrtn( VoxelGrid *sceneWorldVoxelGrid, const ArrayList<Patch *> *scenePatches, RenderOptions *renderOptions);
    static ColorRgb *stchsRelaxRadElemRadn(const StochasticRadiosityElement *elem);
    static void stchsRelaxRadElemUpdRadn(StochasticRadiosityElement *elem, double w);
    static void stchsRelaxRadPrintRegStats();
    static void stchsRelaxRadDRegRadnItrtn( VoxelGrid *sceneWorldVoxelGrid, const ArrayList<Patch *> *scenePatches, RenderOptions *renderOptions);
    static float stchsRelaxRadElemImp(const StochasticRadiosityElement *elem);
    static void stchsRelaxRadElemUpdImp(StochasticRadiosityElement *elem, double w);
    static void stchsRelaxRadDRegImpItrtn( VoxelGrid *sceneWorldVoxelGrid, const ArrayList<Patch *> *scenePatches, RenderOptions *renderOptions);
    static void stchsRelaxRadElemDscrdIncrm(Element *element);
    static void stchsRelaxRadDscrdIncrm();
};

#endif
