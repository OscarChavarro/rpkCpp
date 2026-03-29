#ifndef __MGF_HANDLER_MATERIAL__
#define __MGF_HANDLER_MATERIAL__

#include "material/Material.h"
#include "io/context/MgfParseSession.h"

class ColorContext;
class ColorRgb;

class MgfHandlerMaterial {
  public:
    static int handleMaterialEntity(int ac, const char **av, MgfParseSession *context);
    static void initMaterialContextTables(MgfParseSession *context);
    static int mgfMaterialChanged(const Material *material, const MgfParseSession *context);
    static int mgfGetCurrentMaterial(Material **material, bool allSurfacesSided, MgfParseSession *context);

  private:
    static Material *materialLookup(const char *name, const MgfParseSession *context);
    static void mgfGetColor(ColorContext *cin, float intensity, ColorRgb *colorOut, MgfParseSession *context);
    static void specSamples(const ColorRgb &col, float *rgb);
    static float colorMax(ColorRgb col);
};

#endif
