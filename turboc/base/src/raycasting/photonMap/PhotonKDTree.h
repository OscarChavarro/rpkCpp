/**
Photon kd-tree: specialized kd-tree with some photon map specific additions
*/

#ifndef __PHOTON_K_D_TREE__
#define __PHOTON_K_D_TREE__

#include "common/dataStructures/KDTree.h"
#include "raycasting/photonMap/IrrPhoton.h"
#include "raycasting/photonMap/NormalQuery.h"
class PhotonKDTree: public KDTree{ private:
    static NormalQuery qdat_s;
    static float sqrDistance3D(const float *a, const float *b);
    void NormalBQuery_rec(int index);

  public:
    explicit PhotonKDTree(int dataSize, bool copyData = true);

    IrrPhoton *
    normalPhotonQuery( Vector3D *position, const Vector3D *normal, float threshold, float maxR2);
};

#endif
