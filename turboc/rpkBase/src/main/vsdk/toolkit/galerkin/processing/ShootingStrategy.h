#ifndef __SHOOTING_STRATEGY__
#define __SHOOTING_STRATEGY__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/elements/Patch.h"
#include "vsdk/toolkit/galerkin/GalerkinState.h"

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
    static inline double
    galerkinGetPotential(Patch *patch) {
        return ((GalerkinElement *)(patch->radianceData))->potential;
    }

    static inline double
    galerkinGetUnShotPotential(Patch *patch) {
        return ((GalerkinElement *)(patch->radianceData))->unShotPotential;
    }

    static Patch *
    chooseRadianceShootingPatch(const ArrayList<Patch *> *scenePatches, const GalerkinState *galerkinState);

    static void
    clearUnShotRadianceAndPotential(GalerkinElement *elem);

    static void
    ptchPropUnShotRadnAPot(
        const Scene *scene,
        const Patch *patch,
        GalerkinState *galerkinState);

    static double
    shootingPushPullPotential(GalerkinElement *element, double down);

    static void
    patchUpdateRadianceAndPotential(const Patch *patch, GalerkinState *galerkinState);

    static void
    doPropagate(const Scene *scene, const Patch *shootingPatch, GalerkinState *galerkinState);

    static bool
    propagateRadiance(const Scene *scene, GalerkinState *galerkinState);

    static void
    clusterUpdatePotential(GalerkinElement *clusterElement);

    static Patch *
    choosePotentialShootingPatch(const ArrayList<Patch *> *scenePatches);

    static void propagatePotential(const Scene *scene, GalerkinState *galerkinState);

    static void
    shootingUpdateDirectPotential(GalerkinElement *galerkinElement, double potentialIncrement);

  public:
    static bool doShootingStep(Scene *scene, GalerkinState *galerkinState, const RenderOptions *renderOptions);
};

#endif
