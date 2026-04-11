import { PrintStream } from "../../../../java/io/PrintStream";
import { Matrix2x2 } from "../../common/linealAlgebra/Matrix2x2";

export class Matrix2x2Printer {
  public static print(matrix: Matrix2x2, stream: PrintStream | null): void {
    if (stream === null) {
      return;
    }
    stream.printf("\t%f %f    %f\n", matrix.m[0][0], matrix.m[0][1], matrix.t[0]);
    stream.printf("\t%f %f    %f\n", matrix.m[0][1], matrix.m[1][1], matrix.t[1]);
  }
}
