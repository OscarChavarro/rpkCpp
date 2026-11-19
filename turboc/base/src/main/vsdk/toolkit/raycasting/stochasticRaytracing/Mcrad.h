/**
Monte Carlo radiosity
*/

#ifndef _MONTE_CARLO_RADIOSITY__
#define _MONTE_CARLO_RADIOSITY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3D.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/environment/geometry/elements/Element.h"
#include "vsdk/toolkit/skin/Geometry.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/Vertex.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"

class StochasticRadiosityElement;

class Mcrad{ public:
    static float mntCarloRadSclrRefl(const Patch *patch);
    static void monteCarloRadiosityDefaults();
    static void mntCarloRadUpdCpuSecs();
    static Element *mntCarloRadCreatePtchData(Patch *patch);
    static void mntCarloRadDestroyPtchData(Patch *patch);
    static void mntCarloRadPtchCompNewClr(Patch *patch);
    static void monteCarloRadiosityInit();
    static void mntCarloRadUpdViewImp(Scene *scene, const RenderOptions *renderOptions);
    static void monteCarloRadiosityReInit(Scene *scene, const RenderOptions *renderOptions);
    static void monteCarloRadiosityPreStep(Scene *scene, const RenderOptions *renderOptions);
    static void monteCarloRadiosityTerminate(const ArrayList<Patch *> *scenePatches);
    static ColorRgb monteCarloRadiosityGetRadiance( Patch *patch, double u, double v, Vector3D dir, const RenderOptions *renderOptions);

  private:
    static void monteCarloRadiosityInitPatch(const Patch *patch);
    static void mntCarloRadPullImps(Element *element);
    static void mntCarloRadAccumImps(const StochasticRadiosityElement *elem);
    static void mntCarloRadUpdImp(Element *element);
    static void mntCarloRadReInitImp(Element *element);
    static double mntCarloRadDetAreaFrac( const ArrayList<Patch *> *scenePatches, const ArrayList<Geometry *> *sceneGeometries);
    static void mntCarloRadDetInitNrRays( const ArrayList<Patch *> *scenePatches, const ArrayList<Geometry *> *sceneGeometries);
    static ColorRgb mntCarloRadDffsReflAPnt(Patch *patch, double u, double v);
    static ColorRgb vertexReflectance(const Vertex *vertex);
    static ColorRgb mntCarloRadInterpReflAPnt( const StochasticRadiosityElement *leaf, double u, double v);
};

#endif
