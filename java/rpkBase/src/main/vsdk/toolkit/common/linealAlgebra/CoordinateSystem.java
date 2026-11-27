package vsdk.toolkit.common.linealAlgebra;

import vsdk.toolkit.common.VSDK;

/**
Routines used to sample a spherical triangle or quadrilateral
using Arvo's technique published in SIGGRAPH '95 p 437

References:
- [ARVO1995b] "Stratified Sampling of Spherical Triangles", SIGGRAPH 1995

This class implements local-frame and spherical-coordinate primitives after
Section [ARVO1995b].2, especially the orthogonal normalized component operator from
equation (6). Equations (1)-(5) in the paper concern spherical
triangle area inversion and are not directly implemented here.
*/

/**
Creates a coordinate system with the given UNIT direction vector as inZ-axis
*/

// Equation [ARVO1995b](6): [x|y] = Normalize(x - (x.y) y).

// X is a closed-form tangent orthogonal to Z (equivalent to [worldZ|Z], up to sign).

// Section [ARVO1995b].2: complete a local orthonormal frame to express spherical angles.

/**
Given a unit vector and a coordinate system, this routine computes the spherical
coordinates phi and theta of the vector with respect to the coordinate system
*/

// Sometimes numerical errors cause this

// Equation [ARVO1995b](6): [C|Z] = Normalize(C - (C.Z) Z).

/**
Samples the hemisphere according to a cos_theta distribution
*/

// Section [ARVO1995b].2: map (xi1, xi2) in [0,1]^2 to angular parameters on the sphere.

/**
Samples the hemisphere according to a cos_theta ^ n distribution
*/

// Section [ARVO1995b].2: same square-to-sphere mapping pattern with a different radial CDF.

public class CoordinateSystem {
    private Vector3Dd X = new Vector3Dd();
    private Vector3Dd Y = new Vector3Dd();
    private Vector3Dd Z = new Vector3Dd();

    public Vector3Dd getX() {
        return X;
    }

    public Vector3Dd getY() {
        return Y;
    }

    public Vector3Dd getZ() {
        return Z;
    }

    public void setX(Vector3Dd inX) {
        X = inX;
    }

    public void setY(Vector3Dd inY) {
        Y = inY;
    }

    public void setZ(Vector3Dd inZ) {
        Z = inZ;
    }

    public void setFromZAxis(Vector3Dd inZ) {
        Z = inZ;

        double zz = (double)Math.sqrt(1.0 - inZ.z() * inZ.z());
        if (zz < VSDK.EPSILON) {
            X = X.withX(1.0);
            X = X.withY(0.0);
            X = X.withZ(0.0);
        }
        else {
            X = X.withX(inZ.y() / zz);
            X = X.withY(-inZ.x() / zz);
            X = X.withZ(0.0);
        }

        Y = Z.crossProduct(X);
    }

    public void rectangularToSphericalCoord(Vector3Dd cIn, double[] phi, double[] theta) {
        if (phi == null || phi.length == 0 || theta == null || theta.length == 0) {
            throw new IllegalArgumentException("phi/theta output arrays must have length >= 1");
        }

        double z = cIn.dotProduct(Z);
        if (z > 1.0) {
            z = 1.0;
        }
        if (z < -1.0) {
            z = -1.0;
        }

        theta[0] = Math.acos(z);

        Vector3Dd c = new Vector3Dd();
        c = cIn.add(Z.multiply((double)(-z)));
        c = c.normalized(Numeric.EPSILON);

        double x = c.dotProduct(X);
        double y = c.dotProduct(Y);

        if (x > 1.0) {
            x = 1.0;
        }
        if (x < -1.0) {
            x = -1.0;
        }

        phi[0] = Math.acos(x);
        if (y < 0.0) {
            phi[0] = 2.0 * Math.PI - phi[0];
        }
    }

    public Vector3Dd sampleHemisphereCosTheta(double xi1, double xi2, double[] probabilityDensityFunction) {
        double phi = 2.0 * (double)Math.PI * (double)xi1;
        double cosPhi = (double)Math.cos(phi);
        double sinPhi = (double)Math.sin(phi);
        double cosTheta = (double)Math.sqrt(1.0 - xi2);
        double sinTheta = (double)Math.sqrt(xi2);

        Vector3Dd dir = new Vector3Dd();
        dir = X.multiply(cosPhi).add(Y.multiply(sinPhi));
        dir = dir.multiply(sinTheta).add(Z.multiply(cosTheta));

        if (probabilityDensityFunction != null && probabilityDensityFunction.length > 0) {
            probabilityDensityFunction[0] = cosTheta / Math.PI;
        }

        return dir;
    }

    public Vector3Dd sampleHemisphereCosNTheta(double n, double xi1, double xi2, double[] probabilityDensityFunction) {
        double phi = 2.0 * (double)Math.PI * (double)xi1;
        double cosPhi = (double)Math.cos(phi);
        double sinPhi = (double)Math.sin(phi);
        double cosTheta = (double)Math.pow(xi2, 1.0 / (n + 1.0));
        double sinTheta = (double)Math.sqrt(1.0 - cosTheta * cosTheta);

        Vector3Dd dir = new Vector3Dd();
        dir = X.multiply(cosPhi).add(Y.multiply(sinPhi));
        dir = dir.multiply(sinTheta).add(Z.multiply(cosTheta));

        if (probabilityDensityFunction != null && probabilityDensityFunction.length > 0) {
            probabilityDensityFunction[0] = (n + 1.0) * Math.pow(cosTheta, n) / (2.0 * Math.PI);
        }

        return dir;
    }
}
