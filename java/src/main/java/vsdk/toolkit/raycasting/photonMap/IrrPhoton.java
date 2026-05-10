package vsdk.toolkit.raycasting.photonMap;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector3D;

// IrrPhoton: photon with extra irradiance info
public class IrrPhoton extends Photon {
    public Vector3D m_normal;
    public ColorRgb m_irradiance;

    public IrrPhoton() {
        super();
        m_normal = new Vector3D();
        m_irradiance = new ColorRgb();
    }

    public Vector3D Normal() {
        return m_normal;
    }

    public void setNormal(Vector3D normal) {
        m_normal = new Vector3D(normal.x, normal.y, normal.z);
    }

    public void SetIrradiance(ColorRgb irr) {
        m_irradiance = new ColorRgb(irr.r, irr.g, irr.b);
    }

    public void
    copy(Photon photon) {
        m_pos = new Vector3D(photon.pos().x, photon.pos().y, photon.pos().z);
        m_power = new ColorRgb(photon.power().r, photon.power().g, photon.power().b);
        m_dir = new Vector3D(photon.dir().x, photon.dir().y, photon.dir().z);
    }
}
