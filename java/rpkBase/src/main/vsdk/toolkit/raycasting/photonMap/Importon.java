package vsdk.toolkit.raycasting.photonMap;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

// Importon: identical to IrrPhoton, but with some extra functions
public class Importon extends IrrPhoton {
    public void
    SetAll(double imp, double pot, double foot) {
        // Abuse m_power for importance estimates.
        // -- AT LEAST 3 COLOR components needed!  Watch out with compact photon repr.
        m_power = new ColorRgb(imp, imp, imp);
    }

    public void
    PSetAll(double imp, double pot, double foot) {
        // Abuse m_power for importance estimates.
        // -- AT LEAST 3 COLOR components needed!  Watch out with compact photon repr.
        m_irradiance = new ColorRgb(imp, imp, imp);
    }

    public Importon() {
        super();
    }

    public Importon(
        Vector3Dd pos,
        double importance,
        double potential,
        double footprint,
        Vector3Dd dir)
    {
        super();
        m_pos = new Vector3Dd(pos.x(), pos.y(), pos.z());
        m_dir = new Vector3Dd(dir.x(), dir.y(), dir.z());

        SetAll(importance, potential, footprint);
    }

    public double
    Importance() {
        return (double)m_power.getR();
    }

    public double
    PImportance() {
        return (double)m_irradiance.getR();
    }
}
