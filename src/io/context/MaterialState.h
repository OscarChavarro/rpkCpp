#ifndef __MATERIAL_STATE__
#define __MATERIAL_STATE__

namespace java {
    template <class T>
    class ArrayList;
}

class Material;

class MaterialState {
  public:
    Material *currentMaterial;
    char *currentMaterialName;
    java::ArrayList<Material *> *materials;

    MaterialState();
};

#endif
