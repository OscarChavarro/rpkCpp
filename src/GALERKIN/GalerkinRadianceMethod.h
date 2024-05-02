#ifndef __GALERKIN_RADIOSITY_METHOD__
#define __GALERKIN_RADIOSITY_METHOD__

#include "common/RenderOptions.h"
#include "common/numericalAnalysis/CubatureRule.h"
#include "scene/RadianceMethod.h"
#include "GALERKIN/GalerkinState.h"
#include "GALERKIN/processing/GatheringStrategy.h"
#include "scene/Background.h"
#include "scene/VoxelGrid.h"
#include "scene/Scene.h"

class GalerkinRadianceMethod final : public RadianceMethod {
  private:
    GatheringStrategy *gatheringStrategy;

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
        ((GalerkinElement *)(patch->radianceData))->potential = value;
    }

    static inline void
    galerkinSetUnShotPotential(Patch *patch, float value) {
        ((GalerkinElement *)(patch->radianceData))->unShotPotential = value;
    }

    static void renderElementHierarchy(GalerkinElement *element, const RenderOptions *renderOptions);
    static void galerkinRenderPatch(Patch *patch, const Camera *camera, const RenderOptions *renderOptions);
    static void galerkinDestroyClusterHierarchy(GalerkinElement *clusterElement);

  public:
    static GalerkinState galerkinState;

    static void recomputePatchColor(Patch *patch);

    GalerkinRadianceMethod();
    ~GalerkinRadianceMethod() final;
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
    void writeVRML(const Camera *camera, FILE *fp, const RenderOptions *renderOptions) const final;
};

extern void galerkinFreeMemory();

#endif
