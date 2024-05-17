/**
Photon kd-tree : specialized kd-tree with some photon map specific additions
*/

#ifndef __PHOTON_K_D_TREE__
#define __PHOTON_K_D_TREE__

#include "common/dataStructures/KDTree.h"
#include "PHOTONMAP/photon.h"

class PhotonKDTree final : public KDTree {
  private:
    void NormalBQuery_rec(int index);

  public:
    explicit PhotonKDTree(int dataSize, bool copyData = true);

    CIrrPhoton *
    normalPhotonQuery(
        Vector3D *position,
        const Vector3D *normal,
        float threshold,
        float maxR2);
};

#endif
