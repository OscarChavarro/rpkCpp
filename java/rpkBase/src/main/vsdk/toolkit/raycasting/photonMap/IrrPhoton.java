package vsdk.toolkit.raycasting.photonMap;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

// IrrPhoton: photon with extra irradiance info
public class IrrPhoton extends Photon {
    public Vector3Dd m_normal;
    public ColorRgb m_irradiance;

    public IrrPhoton() {
        super();
        m_normal = new Vector3Dd();
        m_irradiance = new ColorRgb();
    }

    public Vector3Dd Normal() {
        return m_normal;
    }

    public void setNormal(Vector3Dd normal) {
        m_normal = new Vector3Dd(normal.x(), normal.y(), normal.z());
    }

    public void SetIrradiance(ColorRgb irr) {
        m_irradiance = new ColorRgb(irr.getR(), irr.getG(), irr.getB());
    }

    public void
    copy(Photon photon) {
        m_pos = new Vector3Dd(photon.pos().x(), photon.pos().y(), photon.pos().z());
        m_power = new ColorRgb(photon.power().getR(), photon.power().getG(), photon.power().getB());
        m_dir = new Vector3Dd(photon.dir().x(), photon.dir().y(), photon.dir().z());
    }
}
