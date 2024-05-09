#ifndef __SHOOTING__
#define __SHOOTING__

#include "java/util/ArrayList.h"
#include "skin/Patch.h"
#include "GALERKIN/GalerkinState.h"

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
        return ((GalerkinElement *)(patch->radianceData))->potential;
    }

    static inline float
    galerkinGetUnShotPotential(Patch *patch) {
        return ((GalerkinElement *)(patch->radianceData))->unShotPotential;
    }

    static Patch *
    chooseRadianceShootingPatch(const java::ArrayList<Patch *> *scenePatches, const GalerkinState *galerkinState);

    static void
    clearUnShotRadianceAndPotential(GalerkinElement *elem);

    static void
    patchPropagateUnShotRadianceAndPotential(
        const Scene *scene,
        const Patch *patch,
        GalerkinState *galerkinState);

    static float
    shootingPushPullPotential(GalerkinElement *element, float down);

    static void
    patchUpdateRadianceAndPotential(const Patch *patch, GalerkinState *galerkinState);

    static void
    doPropagate(const Scene *scene, const Patch *shootingPatch, GalerkinState *galerkinState);

    static bool
    propagateRadiance(const Scene *scene, GalerkinState *galerkinState);

    static void
    clusterUpdatePotential(GalerkinElement *clusterElement);

    static Patch *
    choosePotentialShootingPatch(const java::ArrayList<Patch *> *scenePatches);

    static void propagatePotential(const Scene *scene, GalerkinState *galerkinState);

    static void
    shootingUpdateDirectPotential(GalerkinElement *galerkinElement, float potentialIncrement);

  public:
    static bool doShootingStep(Scene *scene, GalerkinState *galerkinState, const RenderOptions *renderOptions);
};

#endif
