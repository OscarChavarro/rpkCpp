#ifndef MATERIAL_STATE__
#define MATERIAL_STATE__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/material/Material.h"

class MaterialSelectionContext {
  public:
    Material *currentMaterial;
    char *currentMaterialName;
    java::ArrayList<Material *> *materials;

    MaterialSelectionContext();
};

#endif
