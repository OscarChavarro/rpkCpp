/*
 * Photon kdtree : specialized kdtree with some photon map
 * specific additions
 */

#ifndef __PHOTON_K_D_TREE__
#define __PHOTON_K_D_TREE__

#include "common/dataStructures/KDTree.h"
#include "PHOTONMAP/photon.h"

class CPhotonkdtree : public KDTree {
public:
    explicit CPhotonkdtree(int dataSize, bool CopyData = true)
            : KDTree(dataSize, CopyData) {};

    // normalPhotonQuery : find the nearest photon with a similar normal constraint
    // returns nullptr is no appropriate photon was found (should barely ever happen)
    virtual CIrrPhoton *normalPhotonQuery(Vector3D *position, const Vector3D *normal,
                                          float threshold, float maxR2);

protected:
    void NormalBQuery_rec(int index);
};

#endif
