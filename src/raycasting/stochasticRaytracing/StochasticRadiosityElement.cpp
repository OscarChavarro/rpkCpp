#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"

StochasticRadiosityElement::StochasticRadiosityElement():
    ray_index(),
    quality(),
    prob(),
    ng(),
    area(),
    basis(),
    rad(),
    unShotRad(),
    receivedRad(),
    imp(),
    unShotImp(),
    received_imp(),
    source_imp(),
    imp_ray_index(),
    vertex(),
    parent(),
    regularSubElements(),
    irregularSubElements(),
    upTrans(),
    childNumber(),
    numberOfVertices(),
    isCluster()
{
    className = ElementTypes::ELEMENT_STOCHASTIC_RADIOSITY;
};

StochasticRadiosityElement::~StochasticRadiosityElement() {
}
