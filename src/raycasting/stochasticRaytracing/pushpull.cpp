/*
push/pull operations
 */

#include "common/error.h"
#include "raycasting/stochasticRaytracing/elementmcrad.h"

#define RegularChild(child) (child->child_nr >= 0 && child->child_nr <= 3)

void PushRadiance(ELEMENT *parent, ELEMENT *child, COLOR *parent_rad, COLOR *child_rad) {
    if ( parent->iscluster || child->basis->size == 1 ) {
        colorAdd(child_rad[0], parent_rad[0], child_rad[0]);
    } else if ( RegularChild(child) && child->basis == parent->basis ) {
        FilterColorDown(parent_rad, &(*child->basis->regular_filter)[child->child_nr], child_rad, child->basis->size);
    } else {
        Fatal(-1, "PushRadiance",
              "Not implemented for higher order approximations on irregular child elements or for different parent and child basis");
    }
}

void PushImportance(ELEMENT *parent, ELEMENT *child, float *parent_imp, float *child_imp) {
    *child_imp += *parent_imp;
}

void PullRadiance(ELEMENT *parent, ELEMENT *child, COLOR *parent_rad, COLOR *child_rad) {
    float areafactor = child->area / parent->area;
    if ( parent->iscluster || child->basis->size == 1 ) {
        COLORADDSCALED(parent_rad[0], areafactor, child_rad[0], parent_rad[0]);
    } else if ( RegularChild(child) && child->basis == parent->basis ) {
        FilterColorUp(child_rad, &(*child->basis->regular_filter)[child->child_nr],
                      parent_rad, child->basis->size, areafactor);
    } else {
        Fatal(-1, "PullRadiance",
              "Not implemented for higher order approximations on irregular child elements or for different parent and child basis");
    }
}

void PullImportance(ELEMENT *parent, ELEMENT *child, float *parent_imp, float *child_imp) {
    *parent_imp += child->area / parent->area * (*child_imp);
}
