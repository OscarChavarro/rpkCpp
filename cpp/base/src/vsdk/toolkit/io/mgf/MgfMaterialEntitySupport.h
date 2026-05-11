#ifndef MGF_HANDLER_MATERIAL__
#define MGF_HANDLER_MATERIAL__

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/io/context/ColorContext.h"
#include "vsdk/toolkit/material/Material.h"
#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"

class MgfMaterialEntitySupport {
  public:
    static int handleMaterialEntity(int ac, const char **av, ParseRuntimeContext *context);
    static void initMaterialContextTables(ParseRuntimeContext *context);
    static bool mgfMaterialChanged(const Material *material, const ParseRuntimeContext *context);
    static bool mgfGetCurrentMaterial(Material **material, bool allSurfacesSided, ParseRuntimeContext *context);

  private:
    static constexpr int NUMBER_OF_SAMPLES = 3;
    static Material *materialLookup(const char *name, const ParseRuntimeContext *context);
    static void mgfGetColor(ColorContext *cin, float intensity, ColorRgb *colorOut, ParseRuntimeContext *context);
    static void specSamples(const ColorRgb &col, float *rgb);
    static float colorMax(ColorRgb col);
};

#endif
