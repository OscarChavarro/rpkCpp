package vsdk.toolkit.common.linealAlgebra;

// Direction should be normalized

public class Ray {
    public Vector3D position;
    public Vector3D direction;

    public Ray() {
        position = new Vector3D();
        direction = new Vector3D();
    }

    public Ray(Vector3D position, Vector3D direction) {
        this.position = position;
        this.direction = direction;
    }
}
