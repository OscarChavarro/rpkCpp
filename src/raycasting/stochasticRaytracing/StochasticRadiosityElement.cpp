#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

StochasticRadiosityElement::StochasticRadiosityElement():
    regularSubElements(),
    irregularSubElements(),
    ray_index(),
    quality(),
    prob(),
    ng(),
    basis(),
    imp(),
    unShotImp(),
    received_imp(),
    source_imp(),
    imp_ray_index(),
    vertex(),
    childNumber(),
    numberOfVertices(),
    isCluster()
{
    className = ElementTypes::ELEMENT_STOCHASTIC_RADIOSITY;
};

StochasticRadiosityElement::~StochasticRadiosityElement() {
}
