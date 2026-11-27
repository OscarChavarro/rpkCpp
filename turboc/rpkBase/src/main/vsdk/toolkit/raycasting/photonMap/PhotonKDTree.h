#ifndef __PHOTON_KD_TREE__
#define __PHOTON_KD_TREE__

#include "vsdk/toolkit/common/dataStructures/KDTree.h"
#include "vsdk/toolkit/raycasting/photonMap/IrrPhoton.h"
#include "vsdk/toolkit/raycasting/photonMap/NormalQuery.h"

class PhotonKDTree {
  private:
    KDTree<Photon> *photons;
    KDTree<IrrPhoton> *irradiancePhotons;
    IrrPhoton **irradianceResults;
    int resultCapacity;

    static double sqrDistance3D(const Vector3Dd &a, const Vector3Dd &b);
    void normalQueryRec(int index, NormalQuery &query);

  public:
    explicit PhotonKDTree(bool irradiance);
    ~PhotonKDTree();

    void addPoint(Photon *photon, short flags);
    void addPoint(IrrPhoton *photon, short flags);
    void balance();
    int query(const Vector3Dd &position, int count, Photon **results, double *distances,
              double maximumDistance, short excludeFlags = 0);
    IrrPhoton *normalPhotonQuery(const Vector3Dd *position, const Vector3Dd *normal,
                                 double threshold, double maxR2);
    void iterateIrradiance(KDTreeVisitor<IrrPhoton> &visitor) {
        irradiancePhotons->iterateNodes(visitor);
    }
};

#endif
