#ifndef __MATERIAL_STATE__
#define __MATERIAL_STATE__

#include "java/util/ArrayList.h"
#include "material/Material.h"

class MaterialSelectionContext {
  public:
    Material *currentMaterial;
    char *currentMaterialName;
    ArrayList<Material *> *materials;

    MaterialSelectionContext();
};

#endif
