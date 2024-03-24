#ifndef __MGF_HANDLER_MATERIAL__
#define __MGF_HANDLER_MATERIAL__

#include "material/Material.h"
#include "io/mgf/MgfContext.h"

extern Material *GLOBAL_mgf_currentMaterial;
extern char *GLOBAL_mgf_currentMaterialName;

extern int handleMaterialEntity(int ac, char **av, MgfContext * /*context*/);
extern void initMaterialContextTables();
extern int mgfMaterialChanged(Material *material);
extern int mgfGetCurrentMaterial(Material **material, bool allSurfacesSided, MgfContext *context);

#endif
