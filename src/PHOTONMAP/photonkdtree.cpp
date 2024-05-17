/**
Implementation of specific kdtree for photonmaps
*/

#include "PHOTONMAP/photonkdtree.h"

class NormalQuery {
public:
    CIrrPhoton *photon;
    float *point;
    Vector3D normal;
    float threshold;
    float maximumDistance;

    NormalQuery(): photon(), point(), normal(), threshold(), maximumDistance() {};
};

static NormalQuery qdat_s;

// Distance calculation COPY FROM kdtree.C !
inline static float sqrDistance3D(const float *a, const float *b) {
    float result;
    float tmp;

    tmp = *(a++) - *(b++);
    result = tmp * tmp;

    tmp = *(a++) - *(b++);
    result += tmp * tmp;

    tmp = *a - *b;
    result += tmp * tmp;

    return result;
}

PhotonKDTree::PhotonKDTree(int dataSize, bool copyData) : KDTree(dataSize, copyData) {
}

void
PhotonKDTree::NormalBQuery_rec(const int index) {
    const BalancedKDTreeNode &node = balancedRootNode[index];
    int discr = node.discriminator();

    float dist;
    int nearIndex;
    int farIndex;

    // Recursive call to the child nodes

    // Test discr (reuse distance)

    if ( index < firstLeaf ) {
        dist = ((float *) node.m_data)[discr] - (qdat_s.point)[discr];

        if ( dist >= 0.0 )  // qdat_s.point[discr] <= ((float *)node->m_data)[discr]
        {
            nearIndex = (index << 1) + 1; // node loson
            farIndex = nearIndex + 1; // node hison
        } else {
            farIndex = (index << 1) + 1; // node loson
            nearIndex = farIndex + 1;// node hison
        }

        // Always call near node recursively

        if ( nearIndex < numBalanced ) {
            NormalBQuery_rec(nearIndex);
        }

        dist *= dist; // Square distance to the separator plane
        if ((farIndex < numBalanced) && (dist < qdat_s.maximumDistance) ) {
            // Discriminator line closer than maxdist : nearer positions can lie
            // on the far side. Or there are still not enough nodes found
            NormalBQuery_rec(farIndex);
        }
    }

    dist = sqrDistance3D((float *) node.m_data, qdat_s.point);

    // Normal constraint
    if ( dist < qdat_s.maximumDistance &&
         (((CIrrPhoton *) node.m_data)->Normal().dotProduct(qdat_s.normal) > qdat_s.threshold ) ) {
        // Replace point if distance < maxdist AND normal is similar
        qdat_s.maximumDistance = dist;
        qdat_s.photon = (CIrrPhoton *) node.m_data;
    }
}

/**
Find the nearest photon with a similar normal constraint
returns nullptr is no appropriate photon was found (should barely ever happen)
*/
CIrrPhoton *
PhotonKDTree::normalPhotonQuery(
    Vector3D *position,
    const Vector3D *normal,
    float threshold,
    float maxR2)
{
    // Fill qdat_s
    qdat_s.photon = nullptr;
    qdat_s.normal = *normal;
    qdat_s.point = (float *)position;
    qdat_s.threshold = threshold;
    qdat_s.maximumDistance = maxR2;

    if ( balancedRootNode && (numberOfNodes > 0) && (numUnbalanced == 0) ) {
        // Find the best photon
        NormalBQuery_rec(0);
    }
    return qdat_s.photon;
}
