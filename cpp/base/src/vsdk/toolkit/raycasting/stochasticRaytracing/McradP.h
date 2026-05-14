#ifndef MONTE_CARLO_RADIOSITY__
#define MONTE_CARLO_RADIOSITY__


#include "vsdk/toolkit/java/util/ArrayList.h"
#include "vsdk/toolkit/scene/Scene.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Mcrad.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Nondiff.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/Coefficientsmcrad.h"
#include "vsdk/toolkit/raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

class McradP final {
  public:
    static inline int numberOfVertices(const StochasticRadiosityElement *elem) {
        return elem->patch->getNumberOfVertices();
    }

    static inline StochasticRadiosityElement *topLevelStochasticRadiosityElement(const Patch *patch) {
        return static_cast<StochasticRadiosityElement *>(patch->getRadianceData());
    }

    static inline ColorRgbMutable *getTopLevelPatchRad(const Patch *patch) {
        return topLevelStochasticRadiosityElement(patch)->radiance;
    }

    static inline ColorRgbMutable *getTopLevelPatchUnShotRad(const Patch *patch) {
        return topLevelStochasticRadiosityElement(patch)->unShotRadiance;
    }

    static inline ColorRgbMutable *getTopLevelPatchReceivedRad(const Patch *patch) {
        return topLevelStochasticRadiosityElement(patch)->receivedRadiance;
    }

    static inline GalerkinBasis *getTopLevelPatchBasis(const Patch *patch) {
        return topLevelStochasticRadiosityElement(patch)->basis;
    }
};

#endif
