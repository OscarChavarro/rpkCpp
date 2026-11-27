package vsdk.toolkit.raycasting.photonMap;

import vsdk.toolkit.common.dataStructures.BalancedKDTreeNode;
import vsdk.toolkit.common.dataStructures.KDTree;
import vsdk.toolkit.common.dataStructures.KDTreeAdapter;
import vsdk.toolkit.common.dataStructures.KDTreeVisitor;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

public final class PhotonKDTree {
    private static final class PhotonAdapter implements KDTreeAdapter<Photon> {
        public Vector3Dd getKDPosition(Photon value) { return value.pos(); }
        public Photon copy(Photon value) { return new Photon(value.pos(), value.power(), value.dir()); }
    }
    private static final class IrrPhotonAdapter implements KDTreeAdapter<IrrPhoton> {
        public Vector3Dd getKDPosition(IrrPhoton value) { return value.pos(); }
        public IrrPhoton copy(IrrPhoton value) {
            IrrPhoton result = new IrrPhoton();
            result.copy(value);
            result.setNormal(value.Normal());
            result.SetIrradiance(value.m_irradiance);
            return result;
        }
    }
    private static final class NormalSearch {
        Vector3Dd point;
        Vector3Dd normal;
        double threshold;
        double maximumDistance;
        IrrPhoton photon;
    }
    private final KDTree<Photon> photons;
    private final KDTree<IrrPhoton> irradiancePhotons;
    private IrrPhoton[] irradianceResults = new IrrPhoton[0];

    public PhotonKDTree(boolean irradiance) {
        photons = irradiance ? null : new KDTree<>(new PhotonAdapter(), true);
        irradiancePhotons = irradiance ? new KDTree<>(new IrrPhotonAdapter(), true) : null;
    }
    public void addPoint(Photon photon, short flags) { photons.addPoint(photon, flags); }
    public void addPoint(IrrPhoton photon, short flags) { irradiancePhotons.addPoint(photon, flags); }
    public void balance() {
        if (irradiancePhotons != null) irradiancePhotons.balance();
        else photons.balance();
    }
    public int query(Vector3Dd position, int count, Photon[] results, double[] distances, double radius, short excluded) {
        if (irradiancePhotons == null) return photons.query(position, count, results, distances, radius, excluded);
        if (count > irradianceResults.length) irradianceResults = new IrrPhoton[count];
        int found = irradiancePhotons.query(position, count, irradianceResults, distances, radius, excluded);
        System.arraycopy(irradianceResults, 0, results, 0, found);
        return found;
    }
    public void iterateIrradiance(KDTreeVisitor<IrrPhoton> visitor) { irradiancePhotons.iterateNodes(visitor); }
    public IrrPhoton normalPhotonQuery(Vector3Dd position, Vector3Dd normal, double threshold, double maxR2) {
        if (irradiancePhotons == null || irradiancePhotons.getBalancedCount() == 0 || irradiancePhotons.getUnbalancedCount() != 0) return null;
        NormalSearch query = new NormalSearch();
        query.point = position;
        query.normal = normal;
        query.threshold = threshold;
        query.maximumDistance = maxR2;
        normalQueryRec(0, query);
        return query.photon;
    }
    private void normalQueryRec(int index, NormalSearch query) {
        BalancedKDTreeNode<IrrPhoton> node = irradiancePhotons.getBalancedNode(index);
        Vector3Dd position = node.data.pos();
        if (index < irradiancePhotons.getFirstLeaf()) {
            int axis = node.discriminator();
            double d = position.component(axis) - query.point.component(axis);
            int near = (index << 1) + 1;
            int far = near + 1;
            if (d < 0) { int temp = near; near = far; far = temp; }
            if (near < irradiancePhotons.getBalancedCount()) normalQueryRec(near, query);
            d *= d;
            if (far < irradiancePhotons.getBalancedCount() && d < query.maximumDistance) normalQueryRec(far, query);
        }
        double d = position.x() - query.point.x();
        double distance = d * d;
        d = position.y() - query.point.y(); distance += d * d;
        d = position.z() - query.point.z(); distance += d * d;
        if (distance < query.maximumDistance && node.data.Normal().dotProduct(query.normal) > query.threshold) {
            query.maximumDistance = distance;
            query.photon = node.data;
        }
    }
}
