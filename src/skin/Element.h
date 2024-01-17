#ifndef RPK_ELEMENT_H
#define RPK_ELEMENT_H

#include "material/color.h"
#include "skin/Patch.h"
#include "skin/Geometry.h"

class Element {
  public:
    int id; // Unique ID number for the element
    COLOR Ed; // Diffuse emittance
    COLOR Rd; // Reflectance
    Patch *patch;
    Geometry *geom;
};

#endif
