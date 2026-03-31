#ifndef __MATERIAL_REPOSITORY__
#define __MATERIAL_REPOSITORY__

#include "io/context/LookUpTable.h"
#include "io/context/MaterialContext.h"

class MaterialRepository {
  public:
    LookUpTable *materialLookUpTable;
    MaterialContext defaultMaterialContext;
    MaterialContext unNamedMaterialContext;
    MaterialContext *currentMaterialContext;

    MaterialRepository();
    ~MaterialRepository();

    void reset();

    MaterialRepository(const MaterialRepository &) = delete;
    MaterialRepository &operator=(const MaterialRepository &) = delete;

  private:
    static MaterialContext createDefaultMgfMaterialContext();
};

#endif
