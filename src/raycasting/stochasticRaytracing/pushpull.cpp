/**
stochasticJacobiPush/pull operations
*/

#include "common/error.h"
#include "raycasting/stochasticRaytracing/elementmcrad.h"

inline bool
regularChild(StochasticRadiosityElement *child) {
    return (child->childNumber >= 0 && child->childNumber <= 3);
}

void
pushRadiance(StochasticRadiosityElement *parent, StochasticRadiosityElement *child, COLOR *parent_rad, COLOR *child_rad) {
    if ( parent->isCluster || child->basis->size == 1 ) {
        colorAdd(child_rad[0], parent_rad[0], child_rad[0]);
    } else if ( regularChild(child) && child->basis == parent->basis ) {
            filterColorDown(parent_rad, &(*child->basis->regular_filter)[child->childNumber], child_rad,
                            child->basis->size);
    } else {
        logFatal(-1, "pushRadiance",
                 "Not implemented for higher order approximations on irregular child elements or for different parent and child basis");
    }
}

void
pushImportance(StochasticRadiosityElement * /*parent*/, StochasticRadiosityElement * /*child*/, const float *parent_imp, float *child_imp) {
    *child_imp += *parent_imp;
}

void
pullRadiance(StochasticRadiosityElement *parent, StochasticRadiosityElement *child, COLOR *parent_rad, COLOR *child_rad) {
    float areafactor = child->area / parent->area;
    if ( parent->isCluster || child->basis->size == 1 ) {
        colorAddScaled(parent_rad[0], areafactor, child_rad[0], parent_rad[0]);
    } else if ( regularChild(child) && child->basis == parent->basis ) {
            filterColorUp(child_rad, &(*child->basis->regular_filter)[child->childNumber],
                          parent_rad, child->basis->size, areafactor);
    } else {
        logFatal(-1, "pullRadiance",
                 "Not implemented for higher order approximations on irregular child elements or for different parent and child basis");
    }
}

void
pullImportance(StochasticRadiosityElement *parent, StochasticRadiosityElement *child, float *parent_imp, const float *child_imp) {
    *parent_imp += child->area / parent->area * (*child_imp);
}
