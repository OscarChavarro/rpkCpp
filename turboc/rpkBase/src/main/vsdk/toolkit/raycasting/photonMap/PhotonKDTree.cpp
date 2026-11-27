#include "vsdk/toolkit/raycasting/photonMap/PhotonKDTree.h"

PhotonKDTree::PhotonKDTree(bool irradiance):
    photons(irradiance ? NULL : new KDTree<Photon>()),
    irradiancePhotons(irradiance ? new KDTree<IrrPhoton>() : NULL),
    irradianceResults(NULL), resultCapacity(0) {
}

PhotonKDTree::~PhotonKDTree() {
    delete photons;
    delete irradiancePhotons;
    delete[] irradianceResults;
}

void PhotonKDTree::addPoint(Photon *photon, short flags) {
    photons->addPoint(photon, flags);
}

void PhotonKDTree::addPoint(IrrPhoton *photon, short flags) {
    irradiancePhotons->addPoint(photon, flags);
}

void PhotonKDTree::balance() {
    if ( irradiancePhotons != NULL ) {
        irradiancePhotons->balance();
    } else {
        photons->balance();
    }
}

int PhotonKDTree::query(const Vector3Dd &position, int count, Photon **results,
                        double *distances, double maximumDistance, short excludeFlags) {
    if ( irradiancePhotons == NULL ) {
        return photons->query(position, count, results, distances, maximumDistance, excludeFlags);
    }
    if ( count > resultCapacity ) {
        IrrPhoton **newResults = new IrrPhoton *[count];
        delete[] irradianceResults;
        irradianceResults = newResults;
        resultCapacity = count;
    }
    const int found = irradiancePhotons->query(position, count, irradianceResults, distances,
                                               maximumDistance, excludeFlags);
    for ( int i = 0; i < found; ++i ) {
        results[i] = irradianceResults[i];
    }
    return found;
}

double PhotonKDTree::sqrDistance3D(const Vector3Dd &a, const Vector3Dd &b) {
    double tmp = a.x() - b.x();
    double result = tmp * tmp;
    tmp = a.y() - b.y();
    result += tmp * tmp;
    tmp = a.z() - b.z();
    result += tmp * tmp;
    return result;
}

void PhotonKDTree::normalQueryRec(int index, NormalQuery &query) {
    const BalancedKDTreeNode<IrrPhoton> &node = irradiancePhotons->getBalancedNode(index);
    const Vector3Dd &position = node.data->getKDPosition();
    const int discriminator = node.discriminator();
    if ( index < irradiancePhotons->getFirstLeaf() ) {
        double distance;
        if ( discriminator == 0 ) distance = position.x() - query.point.x();
        else if ( discriminator == 1 ) distance = position.y() - query.point.y();
        else distance = position.z() - query.point.z();
        int nearIndex = (index << 1) + 1;
        int farIndex = nearIndex + 1;
        if ( distance < 0.0 ) {
            const int temp = nearIndex;
            nearIndex = farIndex;
            farIndex = temp;
        }
        if ( nearIndex < irradiancePhotons->getBalancedCount() ) normalQueryRec(nearIndex, query);
        distance *= distance;
        if ( farIndex < irradiancePhotons->getBalancedCount() && distance < query.maximumDistance ) {
            normalQueryRec(farIndex, query);
        }
    }
    const double distance = sqrDistance3D(position, query.point);
    if ( distance < query.maximumDistance && node.data->Normal().dotProduct(query.normal) > query.threshold ) {
        query.maximumDistance = distance;
        query.photon = node.data;
    }
}

IrrPhoton *PhotonKDTree::normalPhotonQuery(const Vector3Dd *position, const Vector3Dd *normal,
                                          double threshold, double maxR2) {
    NormalQuery query;
    query.photon = NULL;
    query.normal = *normal;
    query.point = *position;
    query.threshold = threshold;
    query.maximumDistance = maxR2;
    if ( irradiancePhotons != NULL && irradiancePhotons->getBalancedCount() > 0 &&
         irradiancePhotons->getUnbalancedCount() == 0 ) {
        normalQueryRec(0, query);
    }
    return query.photon;
}
