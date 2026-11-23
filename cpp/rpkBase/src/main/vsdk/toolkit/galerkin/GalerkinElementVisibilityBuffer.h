#ifndef GALERKIN_ELEMENT_VISIBILITY_BUFFER__
#define GALERKIN_ELEMENT_VISIBILITY_BUFFER__

#include "vsdk/toolkit/render/sgl/SglVisibilityBuffer.h"

class GalerkinElement;

/** Borrows surface elements for the current solve. A null sample is background. */
using GalerkinElementVisibilityBuffer = SglVisibilityBuffer<const GalerkinElement *>;

#endif
