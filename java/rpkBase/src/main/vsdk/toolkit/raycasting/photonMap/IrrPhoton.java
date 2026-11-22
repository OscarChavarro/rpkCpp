package vsdk.toolkit.raycasting.photonMap;

import vsdk.toolkit.common.color.ColorRgbMutable;
import vsdk.toolkit.common.linealAlgebra.Vector3Df;

// IrrPhoton: photon with extra irradiance info
public class IrrPhoton extends Photon {
    public Vector3Df m_normal;
    public ColorRgbMutable m_irradiance;

    public IrrPhoton() {
        super();
        m_normal = new Vector3Df();
        m_irradiance = new ColorRgbMutable();
    }

    public Vector3Df Normal() {
        return m_normal;
    }

    public void setNormal(Vector3Df normal) {
        m_normal = new Vector3Df(normal.x(), normal.y(), normal.z());
    }

    public void SetIrradiance(ColorRgbMutable irr) {
        m_irradiance = new ColorRgbMutable(irr.getR(), irr.getG(), irr.getB());
    }

    public void
    copy(Photon photon) {
        m_pos = new Vector3Df(photon.pos().x(), photon.pos().y(), photon.pos().z());
        m_power = new ColorRgbMutable(photon.power().getR(), photon.power().getG(), photon.power().getB());
        m_dir = new Vector3Df(photon.dir().x(), photon.dir().y(), photon.dir().z());
    }
}
