package vsdk.toolkit.raycasting.photonMap;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.CoordinateSystem;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.material.BsdfComponent;

// Non-compact photon representation
public class Photon {
    protected Vector3Dd m_pos;  // Position: 3 floats, MUST COME FIRST for kd tree storage
    protected ColorRgb m_power;  // power represented by this photon
    //  double m_dcWeight; // Weight for density control
    protected Vector3Dd m_dir;  // Direction

    public Photon() {
        m_pos = new Vector3Dd();
        m_power = new ColorRgb();
        m_dir = new Vector3Dd();
    }

    public Photon(Vector3Dd pos, ColorRgb power, Vector3Dd dir) {
        m_pos = new Vector3Dd(pos.x(), pos.y(), pos.z());
        m_power = new ColorRgb(power.getR(), power.getG(), power.getB());
        m_dir = new Vector3Dd(dir.x(), dir.y(), dir.z());
    }

    public Vector3Dd
    pos() {
        return m_pos;
    }

    public ColorRgb
    power() {
        return m_power;
    }

    public void
    addPower(ColorRgb col) {
        m_power = m_power.add(col);
    }

    public Vector3Dd
    dir() {
        return m_dir;
    }

    // Importance sampling utility functions

    // Find the r,s values in a [0,1[^2 square corresponding to the photon
    public void findRS(double[] r, double[] s, CoordinateSystem coord, byte flag, double n) {
        // Determine angles
        double[] phi = new double[1];
        double[] theta = new double[1];
        // Equation [ARVO1995b](6): rectangularToSphericalCoord projects onto the
        // plane orthogonal to the local Z-axis before recovering (phi, theta).
        coord.rectangularToSphericalCoord(m_dir, phi, theta);

        // Compute r, s
        if ( flag == (byte)BsdfComponent.BRDF_DIFFUSE_COMPONENT ) {
            s[0] = phi[0] / (2.0 * Math.PI);
            double tmp = Math.cos(theta[0]);
            r[0] = -tmp * tmp + 1.0;
        } else if ( flag == (byte)BsdfComponent.BRDF_GLOSSY_COMPONENT ) {
            s[0] = phi[0] / (2.0 * Math.PI);
            r[0] = Math.pow(Math.cos(theta[0]), (double)n + 1.0);
        } else {
            Logger.reportMessage(this, Logger.ERROR, "Photon::findRS", String.format("Component %d not implemented yet", flag));
        }
    }
}
