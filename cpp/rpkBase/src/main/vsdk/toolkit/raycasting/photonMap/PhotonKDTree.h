#ifndef __PHOTON_KD_TREE__
#define __PHOTON_KD_TREE__

#include "vsdk/toolkit/common/dataStructures/KDTree.h"
#include "vsdk/toolkit/raycasting/photonMap/IrrPhoton.h"
#include "vsdk/toolkit/raycasting/photonMap/NormalQuery.h"

class PhotonKDTree final {
  private:
    KDTree<Photon> *photons;
    KDTree<IrrPhoton> *irradiancePhotons;
    IrrPhoton **irradianceResults;
    int resultCapacity;

    static float sqrDistance3D(const Vector3Df &a, const Vector3Df &b);
    void normalQueryRec(int index, NormalQuery &query);

  public:
    explicit PhotonKDTree(bool irradiance);
    ~PhotonKDTree();
    PhotonKDTree(const PhotonKDTree &) = delete;
    PhotonKDTree &operator=(const PhotonKDTree &) = delete;

    void addPoint(Photon *photon, short flags);
    void addPoint(IrrPhoton *photon, short flags);
    void balance();
    int query(const Vector3Df &position, int count, Photon **results, float *distances,
              float maximumDistance, short excludeFlags = 0);
    IrrPhoton *normalPhotonQuery(const Vector3Df *position, const Vector3Df *normal,
                                 float threshold, float maxR2);
    template<typename Visitor>
    void iterateIrradiance(Visitor &visitor) {
        irradiancePhotons->iterateNodes(visitor);
    }
};

#endif
