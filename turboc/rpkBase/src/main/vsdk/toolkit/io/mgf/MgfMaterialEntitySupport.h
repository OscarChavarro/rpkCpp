#ifndef CONSTEXPR
#define CONSTEXPR const
#endif

#ifndef __MGF_MATERIAL_ENTITY_SUPPORT__
#define __MGF_MATERIAL_ENTITY_SUPPORT__

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/io/context/ColorContext.h"
#include "vsdk/toolkit/material/RpkMaterial.h"
#include "vsdk/toolkit/io/context/ParseRuntimeContext.h"

class MgfMaterialEntitySupport {
  public:
    static int handleMaterialEntity(int ac, const char **av, ParseRuntimeContext *context);
    static void initMaterialContextTables(ParseRuntimeContext *context);
    static bool mgfMaterialChanged(const RpkMaterial *material, const ParseRuntimeContext *context);
    static bool mgfGetCurrentMaterial(RpkMaterial **material, bool allSurfacesSided, ParseRuntimeContext *context);

  private:
    #define NUMBER_OF_SAMPLES 3
    static RpkMaterial *materialLookup(const char *name, const ParseRuntimeContext *context);
    static bool clipGamut(double *rgb);
    static void mgfGetColor(ColorContext *cin, double intensity, ColorRgb *colorOut, ParseRuntimeContext *context);
    static void specSamples(const ColorRgb &col, double *rgb);
    static double colorMax(const ColorRgb &col);
};

#endif
