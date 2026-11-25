#ifndef __MATERIAL_SELECTION_CONTEXT__
#define __MATERIAL_SELECTION_CONTEXT__

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
