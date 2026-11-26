#ifndef __GALERKIN_RADIANCE_METHOD__
#define __GALERKIN_RADIANCE_METHOD__

#include <cstdarg>

#include "java/lang/String.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/numericalAnalysis/CubatureRule.h"
#include "vsdk/toolkit/scene/Background.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/scene/SceneQuery.h"
#include "vsdk/toolkit/galerkin/GalerkinState.h"
#include "vsdk/toolkit/galerkin/processing/GatheringStrategy.h"

class GalerkinRadianceMethod final : public RadianceMethod {
  private:
    static constexpr int STRING_LENGTH = 2000;
    GatheringStrategy *gatheringStrategy;
    ColorRgb computePatchRadiance(const Patch *patch, double u, double v) const;

    static void patchInit(Patch *patch, const DirectPotentialTable *directPotentials, FaceColorTable *faceColors);
    static void updateCpuSecs();
    static java::String formatToString(const char *format, va_list arguments);
    static void writeFormatted(const char *format, ...);
    static void appendStatsText(char *buffer, int *offset, const char *format, ...);
    static void writeVertexCoord(const Vector3Dd *point);
    static void writeVertexCoords(Element *element);
    static void writeCoords();
    static void writeVertexColor(const ColorRgb &color);
    static void writeVertexColors(Element *element);
    static void writeVertexColorsTopCluster();
    static void writeColors(const RendererConfiguration *renderOptions);
    static void writeCoordIndex(int index);
    static void writeCoordIndices(Element *element);
    static void writeCoordIndicesTopCluster();
    static java::OutputStream *vrmlOutputStream;
    static int numberOfWrites;
    static int vertexId;

    static inline ColorRgb
    galerkinGetRadiance(Patch *patch) {
        return galerkinState.topLevelElement(patch)->radiance[0];
    }

    static inline void
    galerkinSetRadiance(Patch *patch, const ColorRgb &value) {
        galerkinState.topLevelElement(patch)->radiance[0] = value;
    }

    static inline void
    galerkinSetPotential(Patch *patch, double value) {
        galerkinState.topLevelElement(patch)->potential = value;
    }

    static inline void
    galerkinSetUnShotPotential(Patch *patch, double value) {
        galerkinState.topLevelElement(patch)->unShotPotential = value;
    }

    static void galerkinDestroyClusterHierarchy(GalerkinElement *clusterElement);

  public:
    static GalerkinState galerkinState;
    static void freeMemory();

    static void recomputePatchColor(Patch *patch, FaceColorTable *faceColors);

    GalerkinRadianceMethod();
    ~GalerkinRadianceMethod() final;
    const char *getRadianceMethodName() const final;
    void parseOptions(int *argc, char **argv) final;
    void initialize(Scene *scene, ToneMappingContext *toneMapOptions) final;
    bool doStep(Scene *scene, RendererConfiguration *renderOptions) final;
    void terminate(java::ArrayList<Patch *> *scenePatches) final;
    ColorRgb getRadiance(Camera *camera, const Patch *patch, double u, double v, Vector3Dd dir, const RendererConfiguration *rendererConfiguration) const final;
    Element *createPatchData(Patch *patch) final;
    void destroyPatchData(Patch *patch) final;
    char *getStats() const final;
    void
    writeVRML(
        const Camera *camera,
        java::OutputStream *outputStream,
        const RendererConfiguration *renderOptions) const final;
    void setStrategy();
};

#endif
