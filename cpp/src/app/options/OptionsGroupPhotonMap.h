#ifndef OPTIONS_GROUP_PHOTON_MAP__
#define OPTIONS_GROUP_PHOTON_MAP__

#include "raycasting/photonMap/PhotonMapState.h"

class OptionsGroupPhotonMap final {
  public:
    static void parse(
        int *argc,
        char **argv,
        PhotonMapState &photonMapState);

  private:
    static bool parseBoolInt(int argc, char **argv, int &value);
    static void setIntTrue(int &value);
};

#endif
