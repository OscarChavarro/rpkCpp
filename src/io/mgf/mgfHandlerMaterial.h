#ifndef __MGF_HANDLER_MATERIAL__
#define __MGF_HANDLER_MATERIAL__

#include "material/Material.h"
#include "io/mgf/MgfContext.h"

extern int handleMaterialEntity(int ac, char **av, MgfContext * /*context*/);
extern void initMaterialContextTables(MgfContext *context);
extern int mgfMaterialChanged(Material *material, MgfContext *context);
extern int mgfGetCurrentMaterial(Material **material, bool allSurfacesSided, MgfContext *context);

#endif
