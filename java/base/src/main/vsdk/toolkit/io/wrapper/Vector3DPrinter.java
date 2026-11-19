package vsdk.toolkit.io.wrapper;

import java.io.PrintStream;
import vsdk.toolkit.common.linealAlgebra.Vector3D;

public class Vector3DPrinter {
    public static void print(Vector3D vector, PrintStream stream) {
        if (stream == null) {
            return;
        }
        stream.printf("%g %g %g", vector.x, vector.y, vector.z);
    }
}
