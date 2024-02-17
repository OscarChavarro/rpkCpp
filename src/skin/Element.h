#ifndef __ELEMENT__
#define __ELEMENT__

#include "common/color.h"
#include "skin/Patch.h"
#include "skin/Geometry.h"

enum ElementTypes {
    ELEMENT_GALERKIN,
    ELEMENT_STOCHASTIC_RADIOSITY
};

class Element {
  public:
    int id; // Unique ID number for the element
    COLOR Ed; // Diffuse emittance radiance
    COLOR Rd; // Reflectance
    Patch *patch;
    Geometry *geom;
    ElementTypes className;

    Element(): id(), Ed(), Rd(), patch(), geom(), className() {}
};

#endif
