package vsdk.toolkit.io.wrapper;

import java.io.PrintStream;

import vsdk.toolkit.common.linealAlgebra.Matrix2x2;

public final class Matrix2x2Printer {
    private Matrix2x2Printer() {
    }

    public static void print(Matrix2x2 matrix, PrintStream stream) {
        if ( stream == null ) {
            return;
        }
        stream.printf("\t%f %f    %f\n", matrix.get(0, 0), matrix.get(0, 1), matrix.getTranslation(0));
        stream.printf("\t%f %f    %f\n", matrix.get(1, 0), matrix.get(1, 1), matrix.getTranslation(1));
    }
}
