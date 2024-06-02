#ifndef __GALERKING_ELEMENT_DEBUG__
#define __GALERKING_ELEMENT_DEBUG__

#include "GALERKIN/GalerkinElement.h"

class GalerkingElementDebug {
  public:
    static void printGalerkinElementHierarchy(const GalerkinElement *galerkinElement, int level);
};


#endif
