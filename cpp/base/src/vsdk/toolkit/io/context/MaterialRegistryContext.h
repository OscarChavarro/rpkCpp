#ifndef MATERIAL_REPOSITORY__
#define MATERIAL_REPOSITORY__

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

    MaterialRegistryContext(const MaterialRegistryContext &) = delete;
    MaterialRegistryContext &operator=(const MaterialRegistryContext &) = delete;

  private:
    static MaterialContext createDefaultMgfMaterialContext();
};

#endif
