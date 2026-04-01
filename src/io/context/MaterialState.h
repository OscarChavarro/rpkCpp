#ifndef __MATERIAL_STATE__
#define __MATERIAL_STATE__

#include "java/util/ArrayList.h"

class Material;

class MaterialState {
  public:
    Material *currentMaterial;
    char *currentMaterialName;
    java::ArrayList<Material *> *materials;

    MaterialState();
};

#endif
