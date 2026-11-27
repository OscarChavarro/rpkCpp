/**
Monte Carlo radiosity
*/

#ifndef __MCRAD__
#define __MCRAD__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/material/RendererConfiguration.h"
#include "vsdk/toolkit/environment/geometry/elements/Element.h"
#include "vsdk/toolkit/skin/RpkGeometry.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/environment/geometry/elements/RpkVertex.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/scene/RadianceMethod.h"

class StochasticRadiosityElement;

class Mcrad{ public:
    static double mntCarloRadSclrRefl(const Patch *patch);
    static void monteCarloRadiosityDefaults();
    static void mntCarloRadUpdCpuSecs();
    static Element *mntCarloRadCreatePtchData(Patch *patch);
    static void mntCarloRadDestroyPtchData(Patch *patch);
    static void mntCarloRadPtchCompNewClr(const Patch *patch, FaceColorTable *faceColors);
    static void monteCarloRadiosityInit();
    static void mntCarloRadUpdViewImp(Scene *scene, const RenderOptions *renderOptions);
    static void monteCarloRadiosityReInit(Scene *scene, const RenderOptions *renderOptions);
    static void monteCarloRadiosityPreStep(Scene *scene, const RenderOptions *renderOptions);
    static void monteCarloRadiosityTerminate(const ArrayList<Patch *> *scenePatches);
    static ColorRgb monteCarloRadiosityGetRadiance( const Patch *patch, double u, double v, Vector3Dd dir, const RenderOptions *renderOptions);

  private:
    static void monteCarloRadiosityInitPatch(const Patch *patch);
    static void mntCarloRadPullImps(Element *element);
    static void mntCarloRadAccumImps(const StochasticRadiosityElement *elem);
    static bool mntCarloRadVisitChildren(
        const Element *element,
        void (*visit)(Element *, const FaceVisibilityTable *),
        const FaceVisibilityTable *faceVisibility);
    static void mntCarloRadUpdImp(Element *element, const FaceVisibilityTable *faceVisibility);
    static void mntCarloRadReInitImp(Element *element, const FaceVisibilityTable *faceVisibility);
    static double mntCarloRadDetAreaFrac( const ArrayList<Patch *> *scenePatches, const ArrayList<RpkGeometry *> *sceneGeometries);
    static void mntCarloRadDetInitNrRays( const ArrayList<Patch *> *scenePatches, const ArrayList<RpkGeometry *> *sceneGeometries);
    static ColorRgb mntCarloRadDffsReflAPnt(const Patch *patch, double u, double v);
    static ColorRgb vertexReflectance(const RpkVertex *vertex);
    static ColorRgb mntCarloRadInterpReflAPnt( const StochasticRadiosityElement *leaf, double u, double v);
};

#endif
