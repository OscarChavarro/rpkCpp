#ifndef __GALERKIN_RADIOSITY_METHOD__
#define __GALERKIN_RADIOSITY_METHOD__

#include <cstdarg>

#include "common/RenderOptions.h"
#include "java/lang/String.h"
#include "scene/Background.h"
#include "scene/RadianceMethod.h"
#include "scene/Scene.h"
#include "scene/VoxelGrid.h"
#include "numericalAnalysis/CubatureRule.h"
#include "galerkin/GalerkinState.h"
#include "galerkin/processing/GatheringStrategy.h"

class GalerkinRadianceMethod final : public RadianceMethod {
  private:
    GatheringStrategy *gatheringStrategy;

    static void patchInit(Patch *patch);
    static void updateCpuSecs();
    static java::lang::String formatToString(const char *format, va_list arguments);
    static void writeFormatted(const char *format, ...);
    static void appendStatsText(char *buffer, int *offset, const char *format, ...);
    static void writeVertexCoord(const Vector3D *point);
    static void writeVertexCoords(Element *element);
    static void writeCoords();
    static void writeVertexColor(const ColorRgb *color);
    static void writeVertexColors(Element *element);
    static void writeVertexColorsTopCluster();
    static void writeColors(const RenderOptions *renderOptions);
    static void writeCoordIndex(int index);
    static void writeCoordIndices(Element *element);
    static void writeCoordIndicesTopCluster();
    static java::io::OutputStream *vrmlOutputStream;
    static int numberOfWrites;
    static int vertexId;

    static inline ColorRgb
    galerkinGetRadiance(Patch *patch) {
        return static_cast<GalerkinElement *>(patch->radianceData)->radiance[0];
    }

    static inline void
    galerkinSetRadiance(Patch *patch, ColorRgb value) {
        static_cast<GalerkinElement *>(patch->radianceData)->radiance[0] = value;
    }

    static inline void
    galerkinSetPotential(Patch *patch, float value) {
        static_cast<GalerkinElement *>(patch->radianceData)->potential = value;
    }

    static inline void
    galerkinSetUnShotPotential(Patch *patch, float value) {
        static_cast<GalerkinElement *>(patch->radianceData)->unShotPotential = value;
    }

    static void renderElementHierarchy(const GalerkinElement *element, const RenderOptions *renderOptions);
    static void galerkinDestroyClusterHierarchy(GalerkinElement *clusterElement);

  public:
    static GalerkinState galerkinState;
    static void freeMemory();

    static void recomputePatchColor(Patch *patch);
    static void galerkinRenderPatch(const Patch *patch, const Camera *camera, const RenderOptions *renderOptions);

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
    void
    writeVRML(
        const Camera *camera,
        java::io::OutputStream *outputStream,
        const RenderOptions *renderOptions) const final;
    void setStrategy();
};

#endif
