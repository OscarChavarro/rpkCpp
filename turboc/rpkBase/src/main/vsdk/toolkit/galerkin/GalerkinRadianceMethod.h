#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __GALERKIN_RADIANCE_METHOD__
#define __GALERKIN_RADIANCE_METHOD__

#include <stdarg.h>

#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "java/lang/String.h"
#include "vsdk/toolkit/scene/Background.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/scene/SceneQuery.h"
#include "vsdk/toolkit/numericalAnalysis/CubatureRule.h"
#include "vsdk/toolkit/galerkin/GalerkinState.h"
#include "vsdk/toolkit/galerkin/processing/GatheringStrategy.h"

class GalerkinRadianceMethod: public RadianceMethod{ private:
    #define STRING_LENGTH 2000
    GatheringStrategy *gatheringStrategy;
    ColorRgbMutable computePatchRadiance(const Patch *patch, double u, double v) const;

    static void patchInit(Patch *patch, const DirectPotentialTable *directPotentials, FaceColorTable *faceColors);
    static void updateCpuSecs();
    static String formatToString(const char *format, va_list arguments);
    static void writeFormatted(const char *format, ...);
    static void appendStatsText(char *buffer, int *offset, const char *format, ...);
    static void writeVertexCoord(const Vector3Dd *point);
    static void writeVertexCoords(Element *element);
    static void writeCoords();
    static void writeVertexColor(const ColorRgbMutable *color);
    static void writeVertexColors(Element *element);
    static void writeVertexColorsTopCluster();
    static void writeColors(const RenderOptions *renderOptions);
    static void writeCoordIndex(int index);
    static void writeCoordIndices(Element *element);
    static void writeCoordIndicesTopCluster();
    static OutputStream *vrmlOutputStream;
    static int numberOfWrites;
    static int vertexId;

    static inline ColorRgbMutable
    galerkinGetRadiance(Patch *patch){ return galerkinState.topLevelElement(patch)->radiance[0];
    }

    static inline void
    galerkinSetRadiance(Patch *patch, ColorRgbMutable value){ galerkinState.topLevelElement(patch)->radiance[0] = value;
    }

    static inline void
    galerkinSetPotential(Patch *patch, double value){ galerkinState.topLevelElement(patch)->potential = value;
    }

    static inline void
    galerkinSetUnShotPotential(Patch *patch, double value){ galerkinState.topLevelElement(patch)->unShotPotential = value;
    }

    static void galerkinDestroyClusterHierarchy(GalerkinElement *clusterElement);

  public:
    static GalerkinState galerkinState;
    static void freeMemory();

    static void recomputePatchColor(Patch *patch, FaceColorTable *faceColors);

    GalerkinRadianceMethod();
    ~GalerkinRadianceMethod();
    const char *getRadianceMethodName() const;
    void parseOptions(int *argc, char **argv);
    void initialize(Scene *scene, ToneMappingContext *toneMapOptions);
    bool doStep(Scene *scene, RenderOptions *renderOptions);
    void terminate(ArrayList<Patch *> *scenePatches);
    ColorRgbMutable getRadiance(Camera *camera, const Patch *patch, double u, double v, Vector3Dd dir, const RenderOptions *rendererConfiguration) const;
    Element *createPatchData(Patch *patch);
    void destroyPatchData(Patch *patch);
    char *getStats() const;
    void
    writeVRML( const Camera *camera, OutputStream *outputStream, const RenderOptions *renderOptions) const;
    void setStrategy();
};

#endif
