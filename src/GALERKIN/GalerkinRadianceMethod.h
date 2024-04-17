#ifndef __GALERKIN_RADIOSITY_METHOD__
#define __GALERKIN_RADIOSITY_METHOD__

#include "common/numericalAnalysis/cubature.h"
#include "scene/RadianceMethod.h"
#include "GALERKIN/GalerkinState.h"
#include "scene/Background.h"
#include "scene/VoxelGrid.h"

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

    static void renderElementHierarchy(GalerkinElement *element, Camera *camera);
    static void galerkinRenderPatch(Patch *patch, Camera *camera);

  public:
    static GalerkinState galerkinState;

    static void recomputePatchColor(Patch *patch);

    GalerkinRadianceMethod();
    ~GalerkinRadianceMethod();
    const char *getRadianceMethodName() const;
    void parseOptions(int *argc, char **argv);
    void initialize(const java::ArrayList<Patch *> *scenePatches, Geometry *clusteredWorldGeometry);

    int
    doStep(
        Camera *camera,
        Background *sceneBackground,
        java::ArrayList<Patch *> *scenePatches,
        java::ArrayList<Geometry *> *sceneGeometries,
        java::ArrayList<Geometry *> *sceneClusteredGeometries,
        java::ArrayList<Patch *> *lightPatches,
        Geometry *clusteredWorldGeometry,
        VoxelGrid *sceneWorldVoxelGrid);

    void terminate(java::ArrayList<Patch *> *scenePatches);
    ColorRgb getRadiance(Patch *patch, double u, double v, Vector3D dir);
    Element *createPatchData(Patch *patch);
    void destroyPatchData(Patch *patch);
    char *getStats();
    void renderScene(Camera *camera, java::ArrayList<Patch *> *scenePatches, Geometry *clusteredWorldGeometry);
    void writeVRML(Camera *camera, FILE *fp);
};

extern void galerkinFreeMemory();


#endif
