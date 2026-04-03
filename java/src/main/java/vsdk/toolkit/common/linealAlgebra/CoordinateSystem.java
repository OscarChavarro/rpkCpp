package vsdk.toolkit.common.linealAlgebra;

public class CoordinateSystem {
    private Vector3D X = new Vector3D();
    private Vector3D Y = new Vector3D();
    private Vector3D Z = new Vector3D();

    public Vector3D getX() {
        return X;
    }

    public Vector3D getY() {
        return Y;
    }

    public Vector3D getZ() {
        return Z;
    }

    public void setX(Vector3D inX) {
        X = inX;
    }

    public void setY(Vector3D inY) {
        Y = inY;
    }

    public void setZ(Vector3D inZ) {
        Z = inZ;
    }

    public void setFromZAxis(Vector3D inZ) {
        Z = new Vector3D(inZ.x, inZ.y, inZ.z);

        float zz = (float)Math.sqrt(1.0f - inZ.z * inZ.z);
        if (zz < Numeric.EPSILON) {
            X.x = 1.0f;
            X.y = 0.0f;
            X.z = 0.0f;
        }
        else {
            X.x = inZ.y / zz;
            X.y = -inZ.x / zz;
            X.z = 0.0f;
        }

        Y.crossProduct(Z, X);
    }

    public void rectangularToSphericalCoord(Vector3D cIn, double[] phi, double[] theta) {
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

        Vector3D c = new Vector3D();
        c.sumScaled(cIn, -z, Z);
        c.normalize(Numeric.EPSILON_FLOAT);

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

    public Vector3D sampleHemisphereCosTheta(double xi1, double xi2, double[] probabilityDensityFunction) {
        float phi = 2.0f * (float)Math.PI * (float)xi1;
        float cosPhi = (float)Math.cos(phi);
        float sinPhi = (float)Math.sin(phi);
        float cosTheta = (float)Math.sqrt(1.0 - xi2);
        float sinTheta = (float)Math.sqrt(xi2);

        Vector3D dir = new Vector3D();
        dir.combine(cosPhi, X, sinPhi, Y);
        dir.combine(sinTheta, dir, cosTheta, Z);

        if (probabilityDensityFunction != null && probabilityDensityFunction.length > 0) {
            probabilityDensityFunction[0] = cosTheta / Math.PI;
        }

        return dir;
    }

    public Vector3D sampleHemisphereCosNTheta(double n, double xi1, double xi2, double[] probabilityDensityFunction) {
        float phi = 2.0f * (float)Math.PI * (float)xi1;
        float cosPhi = (float)Math.cos(phi);
        float sinPhi = (float)Math.sin(phi);
        float cosTheta = (float)Math.pow(xi2, 1.0 / (n + 1.0));
        float sinTheta = (float)Math.sqrt(1.0 - cosTheta * cosTheta);

        Vector3D dir = new Vector3D();
        dir.combine(cosPhi, X, sinPhi, Y);
        dir.combine(sinTheta, dir, cosTheta, Z);

        if (probabilityDensityFunction != null && probabilityDensityFunction.length > 0) {
            probabilityDensityFunction[0] = (n + 1.0) * Math.pow(cosTheta, n) / (2.0 * Math.PI);
        }

        return dir;
    }
}
