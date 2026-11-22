package vsdk.toolkit.common.linealAlgebra;

import java.io.Serial;
import vsdk.toolkit.common.FundamentalEntity;
import vsdk.toolkit.common.VSDK;

public final class Vector2Df extends FundamentalEntity {
    @Serial
    private static final long serialVersionUID = 20260514L;

    private final float x;
    private final float y;

    public Vector2Df() {
        this(0f, 0f);
    }

    public Vector2Df(float x, float y) {
        this.x = x;
        this.y = y;
    }

    public Vector2Df(Vector2Df other) {
        this(other.x, other.y);
    }

    public Vector2Df(Vector2Dd other) {
        this((float) other.x(), (float) other.y());
    }

    public Vector2Df multiply(float a) {
        return new Vector2Df(a * x, a * y);
    }

    public float length() {
        return (float) Math.sqrt(x * x + y * y);
    }

    public Vector2Df add(Vector2Df b) {
        return new Vector2Df(x + b.x, y + b.y);
    }

    public Vector2Df subtract(Vector2Df b) {
        return new Vector2Df(x - b.x, y - b.y);
    }

    /**
    Square of the length (the dot product with itself)
    */
    public float lengthSquared() {
        return x * x + y * y;
    }

    public Vector2Df withX(float nx) { return new Vector2Df(nx, y); }
    public Vector2Df withY(float ny) { return new Vector2Df(x, ny); }
    public float x() { return x; }
    public float y() { return y; }

    public boolean epsilonEquals(Vector2Df other) {
        return epsilonEquals(other, (float) VSDK.EPSILON);
    }

    public boolean epsilonEquals(Vector2Df other, float epsilon) {
        if ( other == null ) return false;
        if ( epsilon < 0f ) throw new IllegalArgumentException("epsilon must be >= 0");
        return Math.abs(x - other.x) <= epsilon && Math.abs(y - other.y) <= epsilon;
    }
}
