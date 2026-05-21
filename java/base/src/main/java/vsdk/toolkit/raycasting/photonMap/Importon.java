package vsdk.toolkit.raycasting.photonMap;

import vsdk.toolkit.common.linealAlgebra.Vector3D;

// Importon: identical to IrrPhoton, but with some extra functions
public class Importon extends IrrPhoton {
    public void
    SetAll(float imp, float pot, float foot) {
        // Abuse m_power for importance estimates.
        // -- AT LEAST 3 COLOR components needed!  Watch out with compact photon repr.
        m_power.getR() = imp;
    }

    public void
    PSetAll(float imp, float pot, float foot) {
        // Abuse m_power for importance estimates.
        // -- AT LEAST 3 COLOR components needed!  Watch out with compact photon repr.
        m_irradiance.getR() = imp;
    }

    public Importon() {
        super();
    }

    public Importon(
        Vector3D pos,
        float importance,
        float potential,
        float footprint,
        Vector3D dir)
    {
        super();
        m_pos = new Vector3D(pos.x, pos.y, pos.z);
        m_dir = new Vector3D(dir.x, dir.y, dir.z);

        SetAll(importance, potential, footprint);
    }

    public float
    Importance() {
        return (float)m_power.getR();
    }

    public float
    PImportance() {
        return (float)m_irradiance.getR();
    }
}
