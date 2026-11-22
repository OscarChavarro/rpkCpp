import { PrintStream } from "../../../../java/io/PrintStream.js";
import { Matrix2x2 } from "../../common/linealAlgebra/Matrix2x2.js";

export class Matrix2x2Printer {
    public static print(matrix: Matrix2x2, stream: PrintStream | null): void {
        if (stream === null) {
            return;
        }
        stream.printf("\t%f %f    %f\n", matrix.get(0, 0), matrix.get(0, 1), matrix.getTranslation(0));
        stream.printf("\t%f %f    %f\n", matrix.get(1, 0), matrix.get(1, 1), matrix.getTranslation(1));
    }
}
