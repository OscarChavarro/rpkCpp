#ifndef __SHOOTING__
#define __SHOOTING__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"
#include "GALERKIN/GalerkinState.h"
#include "GALERKIN/GalerkinRadianceMethod.h"

/**
Reference:
[COHE1993] Cohen, M. Wallace, J. "Radiosity and Realistic Image Synthesis",
     Academic Press Professional, 1993.
*/

/**
See [COHE1993].5.3.3. section
*/
class ShootingStrategy {
  private:
    static inline float
    galerkinGetPotential(Patch *patch) {
        return ((GalerkinElement *)((patch)->radianceData))->potential;
    }

    static inline float
    galerkinGetUnShotPotential(Patch *patch) {
        return ((GalerkinElement *)((patch)->radianceData))->unShotPotential;
    }

    static Patch *
    chooseRadianceShootingPatch(java::ArrayList<Patch *> *scenePatches, GalerkinState *galerkinState);

    static void
    clearUnShotRadianceAndPotential(GalerkinElement *elem);

    static void
    patchPropagateUnShotRadianceAndPotential(
        Scene *scene,
        Patch *patch,
        GalerkinState *galerkinState,
        RenderOptions *renderOptions);

    static float
    shootingPushPullPotential(GalerkinElement *element, float down);

    static void
    patchUpdateRadianceAndPotential(Patch *patch, GalerkinState *galerkinState);

    static void
    doPropagate(Scene *scene, Patch *shootingPatch, GalerkinState *galerkinState, RenderOptions *renderOptions);

    static bool
    propagateRadiance(Scene *scene, GalerkinState *galerkinState, RenderOptions *renderOptions);

    static void
    clusterUpdatePotential(GalerkinElement *clusterElement);

    static Patch *
    choosePotentialShootingPatch(java::ArrayList<Patch *> *scenePatches);

    static void
    propagatePotential(Scene *scene, GalerkinState *galerkinState, RenderOptions *renderOptions);

    static void
    shootingUpdateDirectPotential(GalerkinElement *elem, float potentialIncrement);

  public:
    static bool doShootingStep(Scene *scene, GalerkinState *galerkinState, RenderOptions *renderOptions);
};

#endif
