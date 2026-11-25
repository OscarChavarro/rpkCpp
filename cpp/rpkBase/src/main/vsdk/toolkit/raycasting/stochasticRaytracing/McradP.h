#ifndef __MCRAD_P__
#define __MCRAD_P__


#include "java/util/ArrayList.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Mcrad.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Nondiff.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Coefficientsmcrad.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRadiosityElement.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/ElementHierarchyState.h"

class McradP final {
  public:
    static inline int numberOfVertices(const StochasticRadiosityElement *elem) {
        return elem->patch->getNumberOfVertices();
    }

    static inline StochasticRadiosityElement *topLevelStochasticRadiosityElement(const Patch *patch) {
        return static_cast<StochasticRadiosityElement *>(
            ElementHierarchyState::activeState().getRadianceElements()->get(patch->getFaceIndex()));
    }

    static inline ColorRgb *getTopLevelPatchRad(const Patch *patch) {
        return topLevelStochasticRadiosityElement(patch)->radiance;
    }

    static inline ColorRgb *getTopLevelPatchUnShotRad(const Patch *patch) {
        return topLevelStochasticRadiosityElement(patch)->unShotRadiance;
    }

    static inline ColorRgb *getTopLevelPatchReceivedRad(const Patch *patch) {
        return topLevelStochasticRadiosityElement(patch)->receivedRadiance;
    }

    static inline StochasticRadiosityBasis *getTopLevelPatchBasis(const Patch *patch) {
        return topLevelStochasticRadiosityElement(patch)->basis;
    }
};

#endif
