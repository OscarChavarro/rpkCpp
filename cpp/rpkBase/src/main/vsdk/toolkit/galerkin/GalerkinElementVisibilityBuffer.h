#ifndef __GALERKIN_ELEMENT_VISIBILITY_BUFFER__
#define __GALERKIN_ELEMENT_VISIBILITY_BUFFER__

#include "vsdk/toolkit/render/sgl/SglVisibilityBuffer.h"

class GalerkinElement;

/** Borrows surface elements for the current solve. A null sample is background. */
// Instantiated once, in GalerkinElementVisibilityBuffer.cpp
extern template class SglVisibilityBuffer<const GalerkinElement *>;
using GalerkinElementVisibilityBuffer = SglVisibilityBuffer<const GalerkinElement *>;

#endif
