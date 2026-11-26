#ifndef __MATERIAL_CONTEXT__
#define __MATERIAL_CONTEXT__

#include "vsdk/toolkit/io/context/ColorContext.h"

class MaterialContext {
  public:
    int clock; // Incremented each change -- resettable
    bool sided; // true if surface is 1-sided, false for 2-sided
    double nr; // Index of refraction, real and imaginary
    double ni;
    double rd; // Diffuse reflectance
    ColorContext rd_c; // Diffuse reflectance color
    double td; // Diffuse transmittance
    ColorContext td_c; // Diffuse transmittance color
    double ed; // Diffuse emittance
    ColorContext ed_c; // Diffuse emittance color
    double rs; // Specular reflectance
    ColorContext rs_c; // Specular reflectance color
    double rs_a; // Specular reflectance roughness
    double ts; // Specular transmittance
    ColorContext ts_c; // Specular transmittance color
    double ts_a; // Specular transmittance roughness
};

#endif
