/**
Data structure for individual photons
*/

#ifndef __PHOTON__
#define __PHOTON__

#include "photonMap/CPhoton.h"
#include "photonMap/CIrrPhoton.h"
#include "photonMap/CImporton.h"
const short DIRECT_LIGHT_PHOTON = 0x10;
const short CAUSTIC_LIGHT_PHOTON = 0x20; // Lower 4 bits reserved for kd tree
// This type of photon should not be included in the importance sampling
const short NO_IMPSAMP_PHOTON = DIRECT_LIGHT_PHOTON | CAUSTIC_LIGHT_PHOTON;


#endif
