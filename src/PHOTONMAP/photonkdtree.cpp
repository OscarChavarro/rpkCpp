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
inline static float sqrDistance3D(float *a, float *b) {
    float result, tmp;

    tmp = *(a++) - *(b++);
    result = tmp * tmp;

    tmp = *(a++) - *(b++);
    result += tmp * tmp;

    tmp = *a - *b;
    result += tmp * tmp;

    return result;
}

void
CPhotonkdtree::NormalBQuery_rec(const int index) {
    const BalancedKDTreeNode &node = m_broot[index];
    int discr = node.Discriminator();

    float dist;
    int nearIndex, farIndex;

    // Recursive call to the child nodes

    // Test discr (reuse dist)

    if ( index < m_firstLeaf ) {
        dist = ((float *) node.m_data)[discr] - (qdat_s.point)[discr];

        if ( dist >= 0.0 )  // qdat_s.point[discr] <= ((float *)node->m_data)[discr]
        {
            nearIndex = (index << 1) + 1; //node->loson;
            farIndex = nearIndex + 1; //node->hison;
        } else {
            farIndex = (index << 1) + 1;//node->loson;
            nearIndex = farIndex + 1;//node->hison;
        }

        // Always call near node recursively

        if ( nearIndex < m_numBalanced ) {
            NormalBQuery_rec(nearIndex);
        }

        dist *= dist; // Square distance to the separator plane
        if ((farIndex < m_numBalanced) && (dist < qdat_s.maximumDistance)) {
            // Discriminator line closer than maxdist : nearer positions can lie
            // on the far side. Or there are still not enough nodes found
            NormalBQuery_rec(farIndex);
        }
    }

    // if(!(node->m_flags & qdat_s.excludeFlags))
    {
        dist = sqrDistance3D((float *) node.m_data, qdat_s.point);

        if ( dist < qdat_s.maximumDistance ) {
            // Normal constraint

            if ((((CIrrPhoton *) node.m_data)->Normal() & qdat_s.normal)
                > qdat_s.threshold ) {
                // Replace point if distance < maxdist AND normal is similar
                qdat_s.maximumDistance = dist;
                qdat_s.photon = (CIrrPhoton *) node.m_data;
            }
        }
    }
}

CIrrPhoton *
CPhotonkdtree::NormalPhotonQuery(
    Vector3D *pos,
    Vector3D *normal,
    float threshold,
    float maxR2)
{
    // Fill qdat_s
    qdat_s.photon = nullptr;
    qdat_s.normal = *normal;
    qdat_s.point = (float *) pos;
    qdat_s.threshold = threshold;
    qdat_s.maximumDistance = maxR2;

    if ( m_broot && (m_numNodes > 0) && (m_numUnbalanced == 0)) {
        // Find the best photon
        NormalBQuery_rec(0);
    }
    return qdat_s.photon;
}
