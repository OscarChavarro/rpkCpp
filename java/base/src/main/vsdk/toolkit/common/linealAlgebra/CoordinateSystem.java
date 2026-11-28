package vsdk.toolkit.common.linealAlgebra;

/**
Routines used to sample a spherical triangle or quadrilateral
using Arvo's technique published in SIGGRAPH '95 p 437

References:
- [ARVO1995b] "Stratified Sampling of Spherical Triangles", SIGGRAPH 1995

This class implements local-frame and spherical-coordinate primitives after
Section [ARVO1995b].2, especially the orthogonal normalized component operator from
equation (6). Equations (1)-(5) in the paper concern spherical
triangle area inversion and are not directly implemented here.

The methods that compute more than one value write them in one-element
arrays given as output holders.
*/
public class CoordinateSystem {
    private Vector3Dd xAxis = new Vector3Dd();
    private Vector3Dd yAxis = new Vector3Dd();
    private Vector3Dd zAxis = new Vector3Dd();

    public Vector3Dd getXAxis() {
        return xAxis;
    }

    public Vector3Dd getYAxis() {
        return yAxis;
    }

    public Vector3Dd getZAxis() {
        return zAxis;
    }

    public void setXAxis(Vector3Dd inX) {
        xAxis = inX;
    }

    public void setYAxis(Vector3Dd inY) {
        yAxis = inY;
    }

    public void setZAxis(Vector3Dd inZ) {
        zAxis = inZ;
    }

    /**
    Creates a coordinate system with the given UNIT direction vector as inZ-axis.
    When inZ is within epsilon of the world Z-axis, world X is taken as X-axis
    */
    public void setFromZAxis(Vector3Dd inZ, double epsilon) {
        zAxis = inZ;

        // Equation [ARVO1995b](6): [x|y] = Normalize(x - (x.y) y).
        // X is a closed-form tangent orthogonal to Z (equivalent to [worldZ|Z], up to sign).
        double zz = (double)Math.sqrt(1.0 - inZ.z() * inZ.z());
        if (zz < epsilon) {
            xAxis = xAxis.withX(1.0);
            xAxis = xAxis.withY(0.0);
            xAxis = xAxis.withZ(0.0);
        }
        else {
            xAxis = xAxis.withX(inZ.y() / zz);
            xAxis = xAxis.withY(-inZ.x() / zz);
            xAxis = xAxis.withZ(0.0);
        }

        // Section [ARVO1995b].2: complete a local orthonormal frame to express spherical angles.
        yAxis = zAxis.crossProduct(xAxis);
    }

    /**
    Given a unit vector and a coordinate system, this routine computes the spherical
    coordinates phi and theta of the vector with respect to the coordinate system.
    epsilon is the length tolerance used to normalize the projection of C onto the
    XY plane
    */
    public void rectangularToSphericalCoord(Vector3Dd cIn, double[] phi, double[] theta, double epsilon) {
        if (phi == null || phi.length == 0 || theta == null || theta.length == 0) {
            throw new IllegalArgumentException("phi/theta output arrays must have length >= 1");
        }

        double z = cIn.dotProduct(zAxis);
        if (z > 1.0) {
            z = 1.0;
        }
        // Sometimes numerical errors cause this
        if (z < -1.0) {
            z = -1.0;
        }

        theta[0] = Math.acos(z);

        // Equation [ARVO1995b](6): [C|Z] = Normalize(C - (C.Z) Z).
        Vector3Dd c = new Vector3Dd();
        c = cIn.add(zAxis.multiply((double)(-z)));
        c = c.normalized(epsilon);

        double x = c.dotProduct(xAxis);
        double y = c.dotProduct(yAxis);

        if (x > 1.0) {
            x = 1.0;
        }
        // Sometimes numerical errors cause this
        if (x < -1.0) {
            x = -1.0;
        }

        phi[0] = Math.acos(x);
        if (y < 0.0) {
            phi[0] = 2.0 * Math.PI - phi[0];
        }
    }

    /**
    Samples the hemisphere according to a cos_theta distribution
    */
    public Vector3Dd sampleHemisphereCosTheta(double xi1, double xi2, double[] probabilityDensityFunction) {
        // Section [ARVO1995b].2: map (xi1, xi2) in [0,1]^2 to angular parameters on the sphere.
        double phi = 2.0 * (double)Math.PI * (double)xi1;
        double cosPhi = (double)Math.cos(phi);
        double sinPhi = (double)Math.sin(phi);
        double cosTheta = (double)Math.sqrt(1.0 - xi2);
        double sinTheta = (double)Math.sqrt(xi2);

        Vector3Dd dir = new Vector3Dd();
        dir = xAxis.multiply(cosPhi).add(yAxis.multiply(sinPhi));
        dir = dir.multiply(sinTheta).add(zAxis.multiply(cosTheta));

        if (probabilityDensityFunction != null && probabilityDensityFunction.length > 0) {
            probabilityDensityFunction[0] = cosTheta / Math.PI;
        }

        return dir;
    }

    /**
    Samples the hemisphere according to a cos_theta ^ n distribution
    */
    public Vector3Dd sampleHemisphereCosNTheta(double n, double xi1, double xi2, double[] probabilityDensityFunction) {
        // Section [ARVO1995b].2: same square-to-sphere mapping pattern with a different radial CDF.
        double phi = 2.0 * (double)Math.PI * (double)xi1;
        double cosPhi = (double)Math.cos(phi);
        double sinPhi = (double)Math.sin(phi);
        double cosTheta = (double)Math.pow(xi2, 1.0 / (n + 1.0));
        double sinTheta = (double)Math.sqrt(1.0 - cosTheta * cosTheta);

        Vector3Dd dir = new Vector3Dd();
        dir = xAxis.multiply(cosPhi).add(yAxis.multiply(sinPhi));
        dir = dir.multiply(sinTheta).add(zAxis.multiply(cosTheta));

        if (probabilityDensityFunction != null && probabilityDensityFunction.length > 0) {
            probabilityDensityFunction[0] = (n + 1.0) * Math.pow(cosTheta, n) / (2.0 * Math.PI);
        }

        return dir;
    }
}
