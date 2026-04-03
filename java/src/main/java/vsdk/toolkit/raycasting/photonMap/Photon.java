package vsdk.toolkit.raycasting.photonMap;

import vsdk.toolkit.common.ColorRgb;
import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.linealAlgebra.CoordinateSystem;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.material.BsdfComponent;

// Non-compact photon representation
public class Photon {
    protected Vector3D m_pos;  // Position: 3 floats, MUST COME FIRST for kd tree storage
    protected ColorRgb m_power;  // power represented by this photon
    //  float m_dcWeight; // Weight for density control
    protected Vector3D m_dir;  // Direction

    public Photon() {
        m_pos = new Vector3D();
        m_power = new ColorRgb();
        m_dir = new Vector3D();
    }

    public Photon(Vector3D pos, ColorRgb power, Vector3D dir) {
        m_pos = new Vector3D(pos.x, pos.y, pos.z);
        m_power = new ColorRgb(power.r, power.g, power.b);
        m_dir = new Vector3D(dir.x, dir.y, dir.z);
    }

    public Vector3D
    pos() {
        return m_pos;
    }

    public ColorRgb
    power() {
        return m_power;
    }

    public void
    addPower(ColorRgb col) {
        m_power.add(m_power, col);
    }

    public Vector3D
    dir() {
        return m_dir;
    }

    // Importance sampling utility functions

    // Find the r,s values in a [0,1[^2 square corresponding to the photon
    public void findRS(double[] r, double[] s, CoordinateSystem coord, byte flag, float n) {
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
            Error.error("Photon::findRS", "Component %d not implemented yet", flag);
        }
    }
}
