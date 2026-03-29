#ifndef __MGF_HANDLER_MATERIAL__
#define __MGF_HANDLER_MATERIAL__

#include "material/Material.h"
#include "io/context/MgfParseSession.h"

extern int handleMaterialEntity(int ac, const char **av, MgfParseSession * /*context*/);
extern void initMaterialContextTables(MgfParseSession *context);
extern int mgfMaterialChanged(const Material *material, const MgfParseSession *context);
extern int mgfGetCurrentMaterial(Material **material, bool allSurfacesSided, MgfParseSession *context);

#endif
