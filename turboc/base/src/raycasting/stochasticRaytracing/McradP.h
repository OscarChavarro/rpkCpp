#ifndef __MONTE_CARLO_RADIOSITY__
#define __MONTE_CARLO_RADIOSITY__

#include "java/util/ArrayList.h"
#include "scene/Scene.h"
#include "raycasting/stochasticRaytracing/Mcrad.h"
#include "raycasting/stochasticRaytracing/Nondiff.h"
#include "raycasting/stochasticRaytracing/Coefficientsmcrad.h"
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

class McradP{ public:
    static inline int numberOfVertices(const StochasticRadiosityElement *elem){ return elem->patch->numberOfVertices;
    }

    static inline StochasticRadiosityElement *topLvlStochRadElem(const Patch *patch){ return ((StochasticRadiosityElement *)(patch->radianceData));
    }

    static inline ColorRgbMutable *getTopLevelPatchRad(const Patch *patch){ return topLvlStochRadElem(patch)->radiance;
    }

    static inline ColorRgbMutable *getTopLevelPatchUnShotRad(const Patch *patch){ return topLvlStochRadElem(patch)->unShotRadiance;
    }

    static inline ColorRgbMutable *getTopLevelPatchReceivedRad(const Patch *patch){ return topLvlStochRadElem(patch)->receivedRadiance;
    }

    static inline GalerkinBasis *getTopLevelPatchBasis(const Patch *patch){ return topLvlStochRadElem(patch)->basis;
    }
};

#endif
