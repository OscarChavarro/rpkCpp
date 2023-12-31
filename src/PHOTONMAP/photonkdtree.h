/*
 * Photon kdtree : specialized kdtree with some photon map
 * specific additions
 */

#ifndef _PHOTONKDTREE_H_
#define _PHOTONKDTREE_H_

#include "common/dataStructures/KDTree.h"
#include "PHOTONMAP/photon.h"

class CPhotonkdtree : public KDTree {
public:
    CPhotonkdtree(int dataSize, bool CopyData = true)
            : KDTree(dataSize, CopyData) {};

    // NormalPhotonQuery : find the nearest photon with a similar normal constraint
    // returns nullptr is no appropriate photon was found (should barely ever happen)
    virtual CIrrPhoton *NormalPhotonQuery(Vector3D *pos, Vector3D *normal,
                                          float threshold, float maxR2);

protected:
    void NormalBQuery_rec(const int index);
};

#endif /* _PHOTONKDTREE_H_ */
