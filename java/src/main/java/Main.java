import vsdk.toolkit.common.linealAlgebra.Matrix4x4;
import vsdk.toolkit.common.linealAlgebra.Vector3D;

public class Main {
    public static void main(String[] args) {
        Vector3D unitVector = Vector3D.unitX();
        Matrix4x4 identity = Matrix4x4.identity();

        System.out.println("Vector3D: " + unitVector);
        System.out.println("Matrix4x4:");
        System.out.println(identity);
    }
}
