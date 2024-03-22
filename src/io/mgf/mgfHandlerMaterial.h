#ifndef __MGF_HANDLER_MATERIAL__
#define __MGF_HANDLER_MATERIAL__

#include "skin/RadianceMethod.h"

extern Material *GLOBAL_mgf_currentMaterial;
extern char *GLOBAL_mgf_currentMaterialName;

extern int handleMaterialEntity(int ac, char **av, RadianceMethod * /*context*/);
extern void mgfClearMaterialTables();
extern int mgfMaterialChanged(Material *material);
extern int mgfGetCurrentMaterial(Material **material, bool allSurfacesSided);

#endif
