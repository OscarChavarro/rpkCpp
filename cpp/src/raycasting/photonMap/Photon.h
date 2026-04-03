/**
Photon flags used by the photon map
*/

#ifndef __PHOTON__
#define __PHOTON__

enum PhotonFlags : short {
    DIRECT_LIGHT_PHOTON = 0x10,
    CAUSTIC_LIGHT_PHOTON = 0x20, // Lower 4 bits reserved for kd tree
    // This type of photon should not be included in the importance sampling
    NO_IMPSAMP_PHOTON = DIRECT_LIGHT_PHOTON | CAUSTIC_LIGHT_PHOTON
};


#endif
