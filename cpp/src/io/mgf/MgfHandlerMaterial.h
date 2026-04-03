#ifndef __MGF_HANDLER_MATERIAL__
#define __MGF_HANDLER_MATERIAL__

#include "common/ColorRgb.h"
#include "io/context/ColorContext.h"
#include "material/Material.h"
#include "io/context/ParseSession.h"

class MgfHandlerMaterial {
  public:
    static int handleMaterialEntity(int ac, const char **av, ParseSession *context);
    static void initMaterialContextTables(ParseSession *context);
    static bool mgfMaterialChanged(const Material *material, const ParseSession *context);
    static bool mgfGetCurrentMaterial(Material **material, bool allSurfacesSided, ParseSession *context);

  private:
    static constexpr int NUMBER_OF_SAMPLES = 3;
    static Material *materialLookup(const char *name, const ParseSession *context);
    static void mgfGetColor(ColorContext *cin, float intensity, ColorRgb *colorOut, ParseSession *context);
    static void specSamples(const ColorRgb &col, float *rgb);
    static float colorMax(ColorRgb col);
};

#endif
