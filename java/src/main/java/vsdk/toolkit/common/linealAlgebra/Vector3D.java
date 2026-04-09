package vsdk.toolkit.common.linealAlgebra;

/**
Fills in x, y, and z component of a vector
*/

/**
Copies the vector v to d: d = v. They may be different vector types
*/

/**
Tolerance value for e.g. a vertex position
*/

/**
Two vectors are equal if their components are equal within the given tolerance
*/

/**
Vector difference
*/

/**
Vector sum: d = a + b
*/

/**
Scaled vector sum: d = a + s.b
*/

/**
Scalar vector product: a.b
*/

/**
Compute (T * vector) with
T = transpose[ X Y Z ] so that e.g. T.X = (1 0 0) if
X, Y, Z form a coordinate system
*/

/**
Square of vector norm: scalar product with itself
*/

/**
Norm of a vector: square root of the square norm
*/

/**
Scale a vector: d = s.v (s is a real number)
*/

/**
Scales a vector with the inverse of the real number s if not zero: d = (1/s).v
*/

/**
Normalizes a vector: scale it with the inverse of its norm
*/

/**
In product of two vectors
*/

/**
Linear combination of two vectors: d = a.v + b.w
*/

/**
Affine linear combination of two vectors: d = o + a.v + b.w
*/

/**
Triple (cross) product: d = (v3 - v2) x (v1 - v2)
*/

/**
Distance between two positions in 3D space: s = | p2 - p1 |
*/

/**
Squared distance between two positions in 3D space: s = |p2-p1|
*/

/**
Centre of two positions
*/

/**
Find the "dominant" part of the vector (eg patch-normal).
This is used to turn the point-in-polygon test into a 2D problem.
*/

public class Vector3D {
    public float x;
    public float y;
    public float z;

    public Vector3D() {
        this(0.0f, 0.0f, 0.0f);
    }

    public Vector3D(float a, float b, float c) {
        x = a;
        y = b;
        z = c;
    }

    public Vector3D(double a, double b, double c) {
        this((float)a, (float)b, (float)c);
    }

    public static Vector3D unitX() {
        return new Vector3D(1.0f, 0.0f, 0.0f);
    }

    public static Vector3D unitY() {
        return new Vector3D(0.0f, 1.0f, 0.0f);
    }

    public static Vector3D unitZ() {
        return new Vector3D(0.0f, 0.0f, 1.0f);
    }

    public Vector3D transform(Vector3D xAxis, Vector3D yAxis, Vector3D zAxis) {
        return new Vector3D(
            xAxis.dotProduct(this),
            yAxis.dotProduct(this),
            zAxis.dotProduct(this));
    }

    public float tolerance(float epsilon) {
        return epsilon * (Math.abs(x) + Math.abs(y) + Math.abs(z));
    }

    public boolean equals(Vector3D w, float epsilon) {
        return Numeric.doubleEqual(x, w.x, epsilon) &&
            Numeric.doubleEqual(y, w.y, epsilon) &&
            Numeric.doubleEqual(z, w.z, epsilon);
    }

    public CoordinateAxis dominantCoordinate() {
        float ax = Math.abs(x);
        float ay = Math.abs(y);
        float az = Math.abs(z);
        float indexValue = Math.max(ax, Math.max(ay, az));

        if (indexValue == ax) {
            return CoordinateAxis.X;
        }
        return indexValue == ay ? CoordinateAxis.Y : CoordinateAxis.Z;
    }

    public float dotProduct(Vector3D b) {
        return x * b.x + y * b.y + z * b.z;
    }

    public float norm2() {
        return x * x + y * y + z * z;
    }

    public float norm() {
        return (float)Math.sqrt(norm2());
    }

    public float distance(Vector3D p2) {
        Vector3D d = new Vector3D();
        d.subtraction(p2, this);
        return d.norm();
    }

    public float distance2(Vector3D p2) {
        Vector3D d = new Vector3D();
        d.subtraction(p2, this);
        return d.norm2();
    }

    public void set(float xParam, float yParam, float zParam) {
        x = xParam;
        y = yParam;
        z = zParam;
    }

    public void copy(Vector3D v) {
        x = v.x;
        y = v.y;
        z = v.z;
    }

    public void combine(float a, Vector3D v, float b, Vector3D w) {
        x = a * v.x + b * w.x;
        y = a * v.y + b * w.y;
        z = a * v.z + b * w.z;
    }

    public void addition(Vector3D a, Vector3D b) {
        x = a.x + b.x;
        y = a.y + b.y;
        z = a.z + b.z;
    }

    public void subtraction(Vector3D a, Vector3D b) {
        x = a.x - b.x;
        y = a.y - b.y;
        z = a.z - b.z;
    }

    public void sumScaled(Vector3D a, double s, Vector3D b) {
        x = a.x + (float)s * b.x;
        y = a.y + (float)s * b.y;
        z = a.z + (float)s * b.z;
    }

    public void scaledCopy(float s, Vector3D v) {
        x = s * v.x;
        y = s * v.y;
        z = s * v.z;
    }

    public void inverseScaledCopy(float s, Vector3D v, float epsilon) {
        float normalizedFactor = (s < -epsilon || s > epsilon) ? 1.0f / s : 1.0f;
        x = normalizedFactor * v.x;
        y = normalizedFactor * v.y;
        z = normalizedFactor * v.z;
    }

    public void normalize(float epsilon) {
        float n = norm();
        inverseScaledCopy(n, this, epsilon);
    }

    public Vector3D normalized() {
        Vector3D out = new Vector3D();
        out.copy(this);
        out.normalize(Numeric.EPSILON_FLOAT);
        return out;
    }

    public void crossProduct(Vector3D a, Vector3D b) {
        x = a.y * b.z - a.z * b.y;
        y = a.z * b.x - a.x * b.z;
        z = a.x * b.y - a.y * b.x;
    }

    public void combine3(Vector3D o, float a, Vector3D v, float b, Vector3D w) {
        x = o.x + a * v.x + b * w.x;
        y = o.y + a * v.y + b * w.y;
        z = o.z + a * v.z + b * w.z;
    }

    public void tripleCrossProduct(Vector3D v1, Vector3D v2, Vector3D v3) {
        Vector3D d1 = new Vector3D();
        Vector3D d2 = new Vector3D();
        d1.subtraction(v3, v2);
        d2.subtraction(v1, v2);
        crossProduct(d1, d2);
    }

    public void midPoint(Vector3D p1, Vector3D p2) {
        x = 0.5f * (p1.x + p2.x);
        y = 0.5f * (p1.y + p2.y);
        z = 0.5f * (p1.z + p2.z);
    }

    @Override
    public String toString() {
        return "Vector3D{" +
            "x=" + x +
            ", y=" + y +
            ", z=" + z +
            '}';
    }
}
