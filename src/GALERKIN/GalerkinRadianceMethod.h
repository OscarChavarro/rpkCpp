#ifndef __GALERKIN_RADIOSITY_METHOD__
#define __GALERKIN_RADIOSITY_METHOD__

#include "common/RenderOptions.h"
#include "common/numericalAnalysis/CubatureRule.h"
#include "scene/RadianceMethod.h"
#include "GALERKIN/GalerkinState.h"
#include "scene/Background.h"
#include "scene/VoxelGrid.h"
#include "scene/Scene.h"

class GalerkinRadianceMethod : public RadianceMethod {
  private:
    static void patchInit(Patch *patch);
    static void updateCpuSecs();

    static inline ColorRgb
    galerkinGetRadiance(Patch *patch) {
        return ((GalerkinElement *)(patch->radianceData))->radiance[0];
    }

    static inline void
    galerkinSetRadiance(Patch *patch, ColorRgb value) {
        ((GalerkinElement *)(patch->radianceData))->radiance[0] = value;
    }

    static inline void
    galerkinSetPotential(Patch *patch, float value) {
        ((GalerkinElement *)((patch)->radianceData))->potential = value;
    }

    static inline void
    galerkinSetUnShotPotential(Patch *patch, float value) {
        ((GalerkinElement *)((patch)->radianceData))->unShotPotential = value;
    }

    static void renderElementHierarchy(GalerkinElement *element, RenderOptions *renderOptions);
    static void galerkinRenderPatch(Patch *patch, Camera *camera, RenderOptions *renderOptions);

  public:
    static GalerkinState galerkinState;

    static void recomputePatchColor(Patch *patch);

    GalerkinRadianceMethod();
    ~GalerkinRadianceMethod();
    const char *getRadianceMethodName() const;
    void parseOptions(int *argc, char **argv);
    void initialize(Scene *scene);
    int doStep(Scene *scene, RenderOptions *renderOptions);
    void terminate(java::ArrayList<Patch *> *scenePatches);
    ColorRgb getRadiance(Camera *camera, Patch *patch, double u, double v, Vector3D dir, RenderOptions *renderOptions);
    Element *createPatchData(Patch *patch);
    void destroyPatchData(Patch *patch);
    char *getStats();
    void renderScene(Scene *scene, RenderOptions *renderOptions);
    void writeVRML(Camera *camera, FILE *fp, RenderOptions *renderOptions);
};

extern void galerkinFreeMemory();

#endif
