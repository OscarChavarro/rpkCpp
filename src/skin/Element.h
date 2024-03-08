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
    Geometry *geometry;
    ElementTypes className;
    COLOR *radiance; // Total radiance on the element as computed so far
    COLOR *receivedRadiance; // Radiance received during iteration
    COLOR *unShotRadiance; // For progressive refinement radiosity

    Element():
        id(),
        Ed(),
        Rd(),
        patch(),
        geometry(),
        className(),
        radiance(),
        receivedRadiance(),
        unShotRadiance()
    {}
    virtual ~Element() {};
};

#endif
