/* phong.h: Phong type EDFs, BRDFs, BTDFs */

#ifndef _PHONG_H_
#define _PHONG_H_

#include "material/color.h"
#include "material/edf_methods.h"
#include "material/brdf_methods.h"
#include "material/btdf_methods.h"

class PHONG_BRDF {
  public:
    COLOR Kd, Ks;
    float avgKd, avgKs;
    float Ns;
};

class PHONG_EDF {
  public:
    COLOR Kd, kd, Ks;
    float Ns;
};

class PHONG_BTDF {
  public:
    COLOR Kd, Ks;
    float avgKd, avgKs;
    float Ns;
    REFRACTIONINDEX refrIndex;
};

/* Define the phong exponent making the difference
   between glossy and highly specular reflection/transmission.
   Choice is arbitrary for the moment */

#define PHONG_LOWEST_SPECULAR_EXP 250
#define PHONG_IS_SPECULAR(p) ((p).Ns >= PHONG_LOWEST_SPECULAR_EXP)

/* creates Phong type EDF, BRDF, BTDF data structs:
 * Kd = diffuse emittance [W/m^2], reflectance or transmittance (number between 0 and 1)
 * Ks = specular emittance, reflectance or transmittance (same dimensions as Kd)
 * Ns = Phong exponent.
 * note: Emittance is total power emitted by the light source per unit of area. */
extern PHONG_EDF *PhongEdfCreate(COLOR *Kd, COLOR *Ks, double Ns);

extern PHONG_BRDF *PhongBrdfCreate(COLOR *Kd, COLOR *Ks, double Ns);

extern PHONG_BTDF *PhongBtdfCreate(COLOR *Kd, COLOR *Ks, double Ns, double nr, double ni);

/* methods for manipulating Phong type EDFs, BRDFs, BTDFs */
extern EDF_METHODS PhongEdfMethods;
extern BRDF_METHODS PhongBrdfMethods;
extern BTDF_METHODS PhongBtdfMethods;

extern void *CreatePhongEdfEditor(void *parent, PHONG_EDF *edf);

extern void *CreatePhongBrdfEditor(void *parent, PHONG_BRDF *brdf);

#endif
