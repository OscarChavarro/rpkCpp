/**
Phong type EDFs, BRDFs, BTDFs
*/

#ifndef __PHONG__
#define __PHONG__

#include "material/color.h"
#include "material/edf_methods.h"
#include "material/brdf_methods.h"
#include "material/btdf_methods.h"

class PHONG_BRDF {
  public:
    COLOR Kd;
    COLOR Ks;
    float avgKd;
    float avgKs;
    float Ns;
};

class PHONG_EDF {
  public:
    COLOR Kd;
    COLOR kd;
    COLOR Ks;
    float Ns;
};

class PHONG_BTDF {
  public:
    COLOR Kd;
    COLOR Ks;
    float avgKd;
    float avgKs;
    float Ns;
    REFRACTIONINDEX refrIndex;
};

/* Define the phong exponent making the difference
   between glossy and highly specular reflection/transmission.
   Choice is arbitrary for the moment */

#define PHONG_LOWEST_SPECULAR_EXP 250
#define PHONG_IS_SPECULAR(p) ((p).Ns >= PHONG_LOWEST_SPECULAR_EXP)

extern PHONG_EDF *phongEdfCreate(COLOR *Kd, COLOR *Ks, double Ns);
extern PHONG_BRDF *phongBrdfCreate(COLOR *Kd, COLOR *Ks, double Ns);
extern PHONG_BTDF *phongBtdfCreate(COLOR *Kd, COLOR *Ks, float Ns, float nr, float ni);

// Methods for manipulating Phong type EDFs, BRDFs, BTDFs
extern EDF_METHODS GLOBAL_scene_phongEdfMethods;
extern BRDF_METHODS GLOBAL_scene_phongBrdfMethods;
extern BTDF_METHODS GLOBAL_scene_phongBtdfMethods;

#endif
