#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __GALERKIN_RADIOSITY_METHOD__
#define __GALERKIN_RADIOSITY_METHOD__

#include <stdarg.h>

#include "common/RenderOptions.h"
#include "java/lang/String.h"
#include "scene/Background.h"
#include "scene/RadianceMethod.h"
#include "scene/Scene.h"
#include "scene/VoxelGrid.h"
#include "numericalAnalysis/CubatureRule.h"
#include "galerkin/GalerkinState.h"
#include "galerkin/processing/GatheringStrategy.h"

class GalerkinRadianceMethod: public RadianceMethod{ private:
    #define STRING_LENGTH 2000
    GatheringStrategy *gatheringStrategy;
    ColorRgb computePatchRadiance(Patch *patch, double u, double v) const;

    static void patchInit(Patch *patch);
    static void updateCpuSecs();
    static String formatToString(const char *format, va_list arguments);
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
    static OutputStream *vrmlOutputStream;
    static int numberOfWrites;
    static int vertexId;

    static inline ColorRgb
    galerkinGetRadiance(Patch *patch){ return ((GalerkinElement *)(patch->radianceData))->radiance[0];
    }

    static inline void
    galerkinSetRadiance(Patch *patch, ColorRgb value){ ((GalerkinElement *)(patch->radianceData))->radiance[0] = value;
    }

    static inline void
    galerkinSetPotential(Patch *patch, float value){ ((GalerkinElement *)(patch->radianceData))->potential = value;
    }

    static inline void
    galerkinSetUnShotPotential(Patch *patch, float value){ ((GalerkinElement *)(patch->radianceData))->unShotPotential = value;
    }

    static void galerkinDestroyClusterHierarchy(GalerkinElement *clusterElement);

  public:
    static GalerkinState galerkinState;
    static void freeMemory();

    static void recomputePatchColor(Patch *patch);

    GalerkinRadianceMethod();
    ~GalerkinRadianceMethod();
    const char *getRadianceMethodName() const;
    void parseOptions(int *argc, char **argv);
    void initialize(Scene *scene, ToneMappingContext *toneMapOptions);
    bool doStep(Scene *scene, RenderOptions *renderOptions);
    void terminate(ArrayList<Patch *> *scenePatches);
    ColorRgb getRadiance(Camera *, Patch *patch, double u, double v, Vector3D, const RenderOptions *) const;
    Element *createPatchData(Patch *patch);
    void destroyPatchData(Patch *patch);
    char *getStats() const;
    void
    writeVRML( const Camera *camera, OutputStream *outputStream, const RenderOptions *renderOptions) const;
    void setStrategy();
};

#endif
