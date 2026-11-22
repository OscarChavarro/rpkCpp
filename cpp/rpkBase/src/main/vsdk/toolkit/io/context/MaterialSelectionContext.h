#ifndef MATERIAL_STATE__
#define MATERIAL_STATE__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/material/RpkMaterial.h"

class MaterialSelectionContext {
  public:
    RpkMaterial *currentMaterial;
    char *currentMaterialName;
    java::ArrayList<RpkMaterial *> *materials;

    MaterialSelectionContext();
};

#endif
