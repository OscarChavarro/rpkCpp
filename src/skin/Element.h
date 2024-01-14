#ifndef RPK_ELEMENT_H
#define RPK_ELEMENT_H

#include "material/color.h"

class Element {
  public:
    COLOR Ed; // Diffuse emittance
    COLOR Rd; // Reflectance
    Patch *patch;
    Geometry *geom;
};

#endif
