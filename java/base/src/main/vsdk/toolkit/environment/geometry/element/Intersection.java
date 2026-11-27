package vsdk.toolkit.environment.geometry.element;

import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

public class Intersection {
    private double t;
    private Vector3Dd point;
    private Vector3Dd normal;

    public Intersection(double t, Vector3Dd point, Vector3Dd normal) {
        this.t = t;
        this.point = point;
        this.normal = normal;
    }

    public double getT() {
        return t;
    }

    public void setT(double value) {
        t = value;
    }

    public Vector3Dd getPoint() {
        return point;
    }

    public void setPoint(Vector3Dd value) {
        point = value;
    }

    public Vector3Dd getNormal() {
        return normal;
    }

    public void setNormal(Vector3Dd value) {
        normal = value;
    }
}
