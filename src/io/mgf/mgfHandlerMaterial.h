#ifndef __MGF_HANDLER_MATERIAL__
#define __MGF_HANDLER_MATERIAL__

#include "skin/RadianceMethod.h"

#define NUMBER_OF_SPECTRAL_SAMPLES 41 // Number of spectral samples

class MgfColorContext {
  public:
    int clock; // Incremented each change
    short flags; // What's been set
    short straightSamples[NUMBER_OF_SPECTRAL_SAMPLES]; // Spectral samples, min wl to max
    long spectralStraightSum; // Straight sum of spectral values
    float cx; // Chromaticity X value
    float cy; // chromaticity Y value
    float eff; // efficacy (lumens / watt)
};

class MgfMaterialContext {
  public:
    int clock; // Incremented each change -- resettable
    int sided; // 1 if surface is 1-sided, 0 for 2-sided
    float nr; // Index of refraction, real and imaginary
    float ni;
    float rd; // Diffuse reflectance
    MgfColorContext rd_c; // Diffuse reflectance color
    float td; // Diffuse transmittance
    MgfColorContext td_c; // Diffuse transmittance color
    float ed; // Diffuse emittance
    MgfColorContext ed_c; // Diffuse emittance color
    float rs; // Specular reflectance
    MgfColorContext rs_c; // Specular reflectance color
    float rs_a; // Specular reflectance roughness
    float ts; // Specular transmittance
    MgfColorContext ts_c; // Specular transmittance color
    float ts_a; // Specular transmittance roughness
};

extern MgfMaterialContext *GLOBAL_mgf_currentMaterial;

extern int handleMaterialEntity(int ac, char **av, RadianceMethod * /*context*/);
extern void mgfClearMaterialTables();
extern int materialChanged(Material *material);
extern int getCurrentMaterial(Material **material, bool allSurfacesSided);

#endif
