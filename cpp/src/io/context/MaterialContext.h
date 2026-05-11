#ifndef MGF_MATERIAL_CONTEXT__
#define MGF_MATERIAL_CONTEXT__

#include "io/context/ColorContext.h"

class MaterialContext {
  public:
    int clock; // Incremented each change -- resettable
    bool sided; // true if surface is 1-sided, false for 2-sided
    float nr; // Index of refraction, real and imaginary
    float ni;
    float rd; // Diffuse reflectance
    ColorContext rd_c; // Diffuse reflectance color
    float td; // Diffuse transmittance
    ColorContext td_c; // Diffuse transmittance color
    float ed; // Diffuse emittance
    ColorContext ed_c; // Diffuse emittance color
    float rs; // Specular reflectance
    ColorContext rs_c; // Specular reflectance color
    float rs_a; // Specular reflectance roughness
    float ts; // Specular transmittance
    ColorContext ts_c; // Specular transmittance color
    float ts_a; // Specular transmittance roughness
};

#endif
