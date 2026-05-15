#ifndef __MATERIAL_REPOSITORY__
#define __MATERIAL_REPOSITORY__

#include "common/dataStructures/LookUpTable.h"
#include "io/context/MaterialContext.h"

class MaterialRegistryContext {
  public:
    LookUpTable<char *> *materialLookUpTable;
    MaterialContext defaultMaterialContext;
    MaterialContext unNamedMaterialContext;
    MaterialContext *currentMaterialContext;

    MaterialRegistryContext();
    ~MaterialRegistryContext();

    void reset();

    MaterialRegistryContext(const MaterialRegistryContext &);
    MaterialRegistryContext &operator=(const MaterialRegistryContext &);

  private:
    static MaterialContext createDefaultMgfMaterialContext();
};

#endif
