#ifndef __MGF_HANDLER_MATERIAL__
#define __MGF_HANDLER_MATERIAL__

#include "material/Material.h"

#include "io/context/BaseContext.h"

extern int handleMaterialEntity(int ac, const char **av, BaseContext * /*context*/);
extern void initMaterialContextTables(BaseContext *context);
extern int mgfMaterialChanged(const Material *material, const BaseContext *context);
extern int mgfGetCurrentMaterial(Material **material, bool allSurfacesSided, BaseContext *context);

#endif
