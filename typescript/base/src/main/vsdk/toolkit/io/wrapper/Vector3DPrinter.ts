import { PrintStream } from "../../../../java/io/PrintStream.js";
import { Vector3Df } from "../../common/linealAlgebra/Vector3Df.js";

export class Vector3DPrinter {
    public static print(vector: Vector3Df, stream: PrintStream | null): void {
        if (stream === null) {
            return;
        }
        stream.printf("%g %g %g", vector.x(), vector.y(), vector.z());
    }
}
