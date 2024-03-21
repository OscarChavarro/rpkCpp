#ifndef __MGF_MATERIAL_CONTEXT__
#define __MGF_MATERIAL_CONTEXT__

#include "MgfColorContext.h"

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

#endif
