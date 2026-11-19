package vsdk.toolkit.common.linealAlgebra;

// TODO: Replace this odd method with standard norm and normalize operations

/**
Routines for 3-d vectors
*/

/**
Normalize a vector, return old magnitude
*/

// Note: this starts being length ^ 2

// First order approximation

/**
Cross product of two vectors
result = a X b
*/

/**
Returns squared distance between the two vectors
*/

public class Vector3Dd {
    public double x;
    public double y;
    public double z;

    public Vector3Dd() {
        x = 0.0;
        y = 0.0;
        z = 0.0;
    }

    public Vector3Dd(double inX, double inY, double inZ) {
        x = inX;
        y = inY;
        z = inZ;
    }

    public double distanceSquared(Vector3Dd v2) {
        Vector3Dd d = new Vector3Dd();
        d.x = v2.x - x;
        d.y = v2.y - y;
        d.z = v2.z - z;
        return d.x * d.x + d.y * d.y + d.z * d.z;
    }

    public double dotProduct(Vector3Dd b) {
        return x * b.x + y * b.y + z * b.z;
    }

    public boolean isNull(double epsilon) {
        return dotProduct(this) <= epsilon * epsilon;
    }

    public double normalizeAndGivePreviousNorm(double epsilon) {
        double len = dotProduct(this);

        if (len <= 0.0) {
            return 0.0;
        }

        if (len <= 1.0 + epsilon && len >= 1.0 - epsilon) {
            len = 0.5 + 0.5 * len;
        }
        else {
            len = Math.sqrt(len);
        }

        x /= len;
        y /= len;
        z /= len;

        return len;
    }

    public void crossProduct(Vector3Dd a, Vector3Dd b) {
        x = a.y * b.z - a.z * b.y;
        y = a.z * b.x - a.x * b.z;
        z = a.x * b.y - a.y * b.x;
    }

    public void copy(Vector3Dd source) {
        x = source.x;
        y = source.y;
        z = source.z;
    }
}
