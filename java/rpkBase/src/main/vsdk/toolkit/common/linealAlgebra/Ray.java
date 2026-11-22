package vsdk.toolkit.common.linealAlgebra;

// Direction should be normalized

public class Ray {
    public Vector3Df position;
    public Vector3Df direction;

    public Ray() {
        position = new Vector3Df();
        direction = new Vector3Df();
    }

    public Ray(Vector3Df position, Vector3Df direction) {
        this.position = position;
        this.direction = direction;
    }
}
