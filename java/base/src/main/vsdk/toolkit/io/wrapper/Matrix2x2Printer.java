package vsdk.toolkit.io.wrapper;

import java.io.PrintStream;
import vsdk.toolkit.common.linealAlgebra.Matrix2x2;

public class Matrix2x2Printer {
    public static void print(Matrix2x2 matrix, PrintStream stream) {
        if (stream == null) {
            return;
        }
        stream.printf("\t%f %f    %f\n", matrix.m[0][0], matrix.m[0][1], matrix.t[0]);
        stream.printf("\t%f %f    %f\n", matrix.m[0][1], matrix.m[1][1], matrix.t[1]);
    }
}
