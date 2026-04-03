/**
Photon kd-tree : specialized kd-tree with some photon map specific additions
*/

package vsdk.toolkit.raycasting.photonMap;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

import vsdk.toolkit.common.dataStructures.KDTree;
import vsdk.toolkit.common.linealAlgebra.Vector3D;

public class PhotonKDTree extends KDTree {
    private static class NodeData {
        Photon photon;
        short flags;
        float[] point;

        NodeData(Photon photon, short flags) {
            this.photon = photon;
            this.flags = flags;
            Vector3D p = photon.pos();
            this.point = new float[] {p.x, p.y, p.z};
        }
    }

    private static class QueryCandidate {
        Photon photon;
        float distance;

        QueryCandidate(Photon photon, float distance) {
            this.photon = photon;
            this.distance = distance;
        }
    }

    private static NormalQuery qdat_s = new NormalQuery();
    private final List<NodeData> nodes;

    // Distance calculation COPY FROM kdtree.C !
    private static float sqrDistance3D(float[] a, float[] b) {
        float result;
        float tmp;

        tmp = a[0] - b[0];
        result = tmp * tmp;

        tmp = a[1] - b[1];
        result += tmp * tmp;

        tmp = a[2] - b[2];
        result += tmp * tmp;

        return result;
    }

    public PhotonKDTree(int dataSize, boolean copyData) {
        super(dataSize, copyData);
        nodes = new ArrayList<>();
    }

    @Override
    public void addPoint(Object data, short flags) {
        if ( !(data instanceof Photon) ) {
            throw new IllegalArgumentException("PhotonKDTree only accepts Photon data");
        }
        nodes.add(new NodeData((Photon)data, flags));
    }

    @Override
    public int query(float[] point, int n, Object[] results) {
        return query(point, n, results, null, KD_MAX_RADIUS, (short)0);
    }

    @Override
    public int query(float[] point, int n, Object[] results, float[] inDistances, float radius, short excludeFlags) {
        List<QueryCandidate> candidates = new ArrayList<>();

        for ( NodeData node : nodes ) {
            if ( (node.flags & excludeFlags) != 0 ) {
                continue;
            }
            float dist = sqrDistance3D(node.point, point);
            if ( dist < radius ) {
                candidates.add(new QueryCandidate(node.photon, dist));
            }
        }

        candidates.sort(Comparator.comparingDouble(c -> c.distance));

        int found = Math.min(n, candidates.size());
        if ( found <= 0 ) {
            return 0;
        }

        float[] usedDistances = inDistances;
        if ( usedDistances == null ) {
            usedDistances = new float[Math.max(1, found)];
        }

        // C kd-tree query keeps the farthest of the selected photons at index 0.
        QueryCandidate farthest = candidates.get(found - 1);
        results[0] = farthest.photon;
        usedDistances[0] = farthest.distance;

        int outIndex = 1;
        for ( int i = 0; i < found - 1; i++ ) {
            results[outIndex] = candidates.get(i).photon;
            usedDistances[outIndex] = candidates.get(i).distance;
            outIndex++;
        }

        return found;
    }

    @Override
    public void iterateNodes(NodeCallback callback, Object data) {
        for ( NodeData node : nodes ) {
            callback.call(data, node.photon);
        }
    }

    @Override
    public void balance() {
        // This Java migration keeps a linear container. No explicit balancing needed.
    }

    /**
Find the nearest photon with a similar normal constraint
returns nullptr is no appropriate photon was found (should barely ever happen)
*/
    public IrrPhoton
    normalPhotonQuery(
        Vector3D position,
        Vector3D normal,
        float threshold,
        float maxR2)
    {
        // Fill qdat_s
        qdat_s.photon = null;
        qdat_s.normal = new Vector3D(normal.x, normal.y, normal.z);
        qdat_s.point = new float[] {position.x, position.y, position.z};
        qdat_s.threshold = threshold;
        qdat_s.maximumDistance = maxR2;

        for ( NodeData node : nodes ) {
            if ( !(node.photon instanceof IrrPhoton) ) {
                continue;
            }

            float dist = sqrDistance3D(node.point, qdat_s.point);

            // Normal constraint
            IrrPhoton photon = (IrrPhoton)node.photon;
            if ( dist < qdat_s.maximumDistance &&
                 (photon.Normal().dotProduct(qdat_s.normal) > qdat_s.threshold ) ) {
                // Replace point if distance < maxdist AND normal is similar
                qdat_s.maximumDistance = dist;
                qdat_s.photon = photon;
            }
        }

        return qdat_s.photon;
    }
}
