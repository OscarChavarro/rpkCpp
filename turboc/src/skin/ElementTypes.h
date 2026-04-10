#ifndef __ELEMENT_TYPES__
#define __ELEMENT_TYPES__

#include "common/VSDK.h"

enum ElementTypes {
    ELEMENT_GALERKIN
#ifdef RAYTRACING_ENABLED
    ,
    ELEMENT_STOCHASTIC_RADIOSITY
#endif
};

#endif
