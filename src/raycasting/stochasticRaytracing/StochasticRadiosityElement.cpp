#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

StochasticRadiosityElement::StochasticRadiosityElement():
        rayIndex(),
        quality(),
        samplingProbability(),
        ng(),
        basis(),
        importance(),
        unShotImportance(),
        receivedImportance(),
        sourceImportance(),
        importanceRayIndex(),
        vertices(),
        childNumber(),
        numberOfVertices(),
        isClusterFlag()
{
    className = ElementTypes::ELEMENT_STOCHASTIC_RADIOSITY;
};

StochasticRadiosityElement::~StochasticRadiosityElement() {
}
