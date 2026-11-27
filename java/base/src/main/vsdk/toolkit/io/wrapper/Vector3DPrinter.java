package vsdk.toolkit.io.wrapper;

import java.io.PrintStream;

import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

public final class Vector3DPrinter {
    private Vector3DPrinter() {
    }

    public static void print(Vector3Dd vector, PrintStream stream) {
        if ( stream == null ) {
            return;
        }
        stream.printf("%g %g %g", vector.x(), vector.y(), vector.z());
    }
}
