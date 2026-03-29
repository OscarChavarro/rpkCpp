#ifndef __MATERIAL_REPOSITORY__
#define __MATERIAL_REPOSITORY__

#include "io/context/LookUpTable.h"
#include "io/mgf/MgfMaterialContext.h"

class MaterialRepository {
  public:
    LookUpTable *materialLookUpTable;
    MgfMaterialContext defaultMaterialContext;
    MgfMaterialContext unNamedMaterialContext;
    MgfMaterialContext *currentMaterialContext;

    MaterialRepository();
    ~MaterialRepository();

    void reset();

    MaterialRepository(const MaterialRepository &) = delete;
    MaterialRepository &operator=(const MaterialRepository &) = delete;

  private:
    static MgfMaterialContext createDefaultMgfMaterialContext();
};

#endif
