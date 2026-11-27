#ifndef __MATERIAL_REGISTRY_CONTEXT__
#define __MATERIAL_REGISTRY_CONTEXT__

#include "vsdk/toolkit/common/dataStructures/LookUpTable.h"
#include "vsdk/toolkit/io/context/MaterialContext.h"

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
